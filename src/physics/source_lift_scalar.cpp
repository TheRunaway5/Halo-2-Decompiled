#include "halo2/source_lift_scalar.hpp"
#include "halo2/source_lift_scalar_storage_tu15.hpp"
#include "halo2/cache.hpp"
#include <array>
#include <cstring>
#include <xmmintrin.h>

namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class NumericScope {
  alignas(16) std::array<unsigned char,108> saved_{};
  unsigned previous_mxcsr_{};
public:
  explicit NumericScope(SourceLiftScalarEnvironment fp) {
    if((fp.x87_control_word&0x3fu)!=0x3fu || (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
      throw FormatError("TU1.5 lift scalar requires a masked native floating-point environment");
    previous_mxcsr_=_mm_getcsr();
    asm volatile("fnsave %0":"=m"(saved_)::"memory");
    auto controlled=saved_;
    std::memcpy(controlled.data(),&fp.x87_control_word,2);
    const auto status=std::uint16_t((fp.x87_status&~0x3fu)|(fp.x87_exception_flags&0x3fu));
    std::memcpy(controlled.data()+4,&status,2);
    constexpr std::uint16_t empty=0xffffu;std::memcpy(controlled.data()+8,&empty,2);
    asm volatile("frstor %0"::"m"(controlled):"memory");
    _mm_setcsr(fp.mxcsr);
  }
  ~NumericScope() {
    asm volatile("frstor %0"::"m"(saved_):"memory");
    _mm_setcsr(previous_mxcsr_);
  }
  NumericScope(const NumericScope&)=delete;
  NumericScope&operator=(const NumericScope&)=delete;
};
// Keep the source operand order too: reassociating a commutative intrinsic can
// select the other NaN payload even when ordinary finite values are identical.
float add(float a,float b){asm volatile("addss %1,%0":"+x"(a):"x"(b));return a;}
float sub(float a,float b){asm volatile("subss %1,%0":"+x"(a):"x"(b));return a;}
float mul(float a,float b){asm volatile("mulss %1,%0":"+x"(a):"x"(b));return a;}
float div(float a,float b){asm volatile("divss %1,%0":"+x"(a):"x"(b));return a;}
// COMISS raises invalid for either NaN kind. C++ relational operators may use
// UCOMISS, which would silently change TU1.5's observable exception flags.
bool above(float a,float b){unsigned char yes;const auto x=_mm_set_ss(a),y=_mm_set_ss(b);asm volatile("comiss %2,%1; seta %0":"=qm"(yes):"x"(x),"x"(y):"cc");return yes;}
bool below_or_unordered(float a,float b){unsigned char yes;const auto x=_mm_set_ss(a),y=_mm_set_ss(b);asm volatile("comiss %2,%1; setb %0":"=qm"(yes):"x"(x),"x"(y):"cc");return yes;}
float absolute(float value){return below_or_unordered(value,0.f)?sub(0.f,value):value;}
float braking_root(float step,float distance){
  // Source FLD32/FMUL32/FADD ST,ST/FSQRT/FSTP32. Keep each x87 rounding
  // point under the supplied precision/control word, including24/64-bit modes.
  float result;
  asm volatile("flds %1; fmuls %2; fadd %%st(0),%%st; fsqrt; fstps %0"
      :"=m"(result):"m"(step),"m"(distance):"st");
  return result;
}
#else
class NumericScope {
public:
  explicit NumericScope(SourceLiftScalarEnvironment){throw FormatError("TU1.5 lift scalar requires the native x87/SSE numeric backend");}
};
float add(float,float);float sub(float,float);float mul(float,float);float div(float,float);
bool above(float,float);float absolute(float);float braking_root(float,float);
#endif
}
SourceLiftScalarOutcome source_lift_scalar_storage_tu15(SourceLiftScalarStorageTu15& s,
    SourceLiftScalarEnvironment fp,const SourceLiftPowerLazyTu15& power){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  NumericScope scope(fp);bool used_x87{};s.begin_local_scratch();
  const auto tested_exponent=s.tested_exponent();
  float acceleration;
  if(above(tested_exponent,s.epsilon(SourceLiftScalarEpsilonUse::Exponent))){
    const auto target=s.target_for_power();const bool negated=below_or_unordered(target,0.f);
    s.power_base(negated?sub(0.f,target):target,negated);
    const float base=s.power_base(),exponent=s.exponent();
    if(!power)throw FormatError("Lift scalar requires original positive-exponent power owner");
    const SourceLiftAccelerationLoad load_acceleration=[&]{return s.acceleration();};
    s.power_acceleration(power(base,exponent,load_acceleration));used_x87=true;
    acceleration=s.power_acceleration();
  }else acceleration=s.acceleration();
  if(s.scale_by_mass()){
    const auto mass=s.mass();if(above(mass,s.epsilon(SourceLiftScalarEpsilonUse::Mass)))acceleration=div(acceleration,mass);
  }
  const float step=mul(acceleration,s.seconds());const bool negative_step=below_or_unordered(step,0.f);
  s.step(step);const float maximum_delta=negative_step?sub(0.f,step):step;
  const bool positive_step=above(step,s.epsilon(SourceLiftScalarEpsilonUse::Step));
  const auto target=s.target_for_distance();const float remaining=sub(target,s.position());
  const float limit=s.velocity_limit(),velocity=s.velocity();s.distance(remaining);
  float desired;
  if(positive_step){
    if(s.force_acceleration())desired=add(step,velocity);
    else {
      const auto ratio=div(velocity,step);const auto shifted=add(ratio,s.one());
      const auto displacement=mul(velocity,s.braking_seconds());const auto product=mul(shifted,displacement);
      const float braking=mul(product,s.half());
      if(above(remaining,braking) && above(remaining,step))desired=add(step,velocity);
      else if(above(0.f,remaining))desired=velocity;
      else {
        const auto stored_step=s.braking_step(),stored_distance=s.braking_distance();
        s.braking_result(braking_root(stored_step,stored_distance));used_x87=true;desired=s.braking_result();
      }
    }
    const auto reverse=s.reverse_minimum();const float minimum=s.allow_reverse()?reverse:velocity;
    if(above(minimum,desired))desired=minimum;
    else if(above(desired,limit))desired=limit;
  }else {
    desired=add(step,velocity);const float minimum=sub(0.f,limit);
    if(above(minimum,desired))desired=minimum;
    else {
      const auto reverse=s.reverse_maximum();const float maximum=s.allow_reverse()?reverse:velocity;
      if(above(desired,maximum))desired=maximum;
    }
  }
  const auto absolute_limit=absolute(limit);
  const float ratio=above(absolute_limit,s.epsilon(SourceLiftScalarEpsilonUse::Ratio))?absolute(div(velocity,limit)):0.f;
  s.prepare_ratio_destination();float delta=sub(desired,velocity);
  const float minimum_delta=sub(0.f,maximum_delta);const bool below_minimum=above(minimum_delta,delta);
  s.ratio(ratio);
  if(below_minimum)delta=minimum_delta;
  else if(above(delta,maximum_delta))delta=maximum_delta;
  s.finish_local_scratch(below_minimum);
  std::uint16_t status,control;asm volatile("fnstsw %0; fnstcw %1":"=am"(status),"=m"(control));
  return {{delta,ratio},_mm_getcsr(),status,used_x87,control};
#else
  (void)s;(void)power;NumericScope scope(fp);return {};
#endif
}
namespace {
struct ScalarInputStorage final:SourceLiftScalarStorageTu15 {
  const SourceLiftScalarInput&i;const SourceLiftScalarConstants&c;float scratch{},scaled{},stored_step{},stored_distance{};
  ScalarInputStorage(const SourceLiftScalarInput&input,const SourceLiftScalarConstants&constants):i(input),c(constants){}
  void begin_local_scratch()override{}
  float tested_exponent()const override{return i.exponent;}
  float epsilon(SourceLiftScalarEpsilonUse)const override{return c.epsilon;}
  float target_for_power()const override{return i.target;}
  void power_base(float value,bool)override{scratch=value;}float power_base()const override{return scratch;}
  float exponent()const override{return i.exponent;}
  float acceleration()const override{return i.acceleration;}
  void power_acceleration(float value)override{scaled=value;}float power_acceleration()const override{return scaled;}
  bool scale_by_mass()const override{return i.scale_by_mass;}float mass()const override{return i.mass;}
  float seconds()override{return i.time_step;}float braking_seconds()const override{return i.time_step;}
  void step(float value)override{stored_step=value;}
  float target_for_distance()const override{return i.target;}float position()const override{return i.position;}
  float velocity_limit()const override{return i.velocity_limit;}float velocity()const override{return i.velocity;}
  void distance(float value)override{stored_distance=value;}bool force_acceleration()const override{return i.force_acceleration;}
  float one()const override{return c.one;}float half()const override{return c.half;}
  float braking_step()const override{return stored_step;}float braking_distance()const override{return stored_distance;}
  void braking_result(float value)override{stored_step=value;}float braking_result()const override{return stored_step;}
  bool allow_reverse()const override{return i.allow_reverse;}float reverse_minimum()const override{return c.reverse_minimum;}
  float reverse_maximum()const override{return c.reverse_maximum;}void prepare_ratio_destination()override{}
  void ratio(float)override{}void finish_local_scratch(bool)override{}
};
}
SourceLiftScalarOutcome source_lift_scalar_tu15(const SourceLiftScalarInput&i,const SourceLiftScalarConstants&c,
    SourceLiftScalarEnvironment fp,const SourceLiftPower&power){
  ScalarInputStorage storage(i,c);SourceLiftPowerLazyTu15 lazy;
  if(power)lazy=[&](float base,float exponent,const SourceLiftAccelerationLoad& acceleration){return power(base,exponent,acceleration());};
  return source_lift_scalar_storage_tu15(storage,fp,lazy);
}
SourceLiftScalarResult source_lift_scalar(const SourceLiftScalarInput&i,
    const SourceLiftScalarConstants&c,const SourceLiftPower&power){
  return source_lift_scalar_tu15(i,c,{},power).result;
}
}
