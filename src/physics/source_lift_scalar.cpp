#include "halo2/source_lift_scalar.hpp"
#include "halo2/cache.hpp"
#include <cmath>

namespace halo2 {
namespace {
float add(float a,float b){volatile float v=a+b;return v;}
float sub(float a,float b){volatile float v=a-b;return v;}
float mul(float a,float b){volatile float v=a*b;return v;}
float div(float a,float b){volatile float v=a/b;return v;}
float absolute(float a){return a>=0.f?a:sub(0.f,a);}
double wide_mul(double a,double b){volatile double v=a*b;return v;}
double wide_add(double a,double b){volatile double v=a+b;return v;}
}
SourceLiftScalarResult source_lift_scalar(const SourceLiftScalarInput& i,
    const SourceLiftScalarConstants& c,const SourceLiftPower& power){
  float acceleration=i.acceleration;
  if(i.exponent>c.epsilon){
    if(!power)throw FormatError("Lift scalar requires original positive-exponent power owner");
    acceleration=power(absolute(i.target),i.exponent,i.acceleration);
  }
  if(i.scale_by_mass && i.mass>c.epsilon)acceleration=div(acceleration,i.mass);
  const float step=mul(acceleration,i.time_step),maximum_delta=absolute(step);
  const float remaining=sub(i.target,i.position);
  float desired;
  if(step>c.epsilon){
    if(i.force_acceleration)desired=add(step,i.velocity);
    else {
      const float braking=mul(mul(add(div(i.velocity,step),c.one),
          mul(i.velocity,i.time_step)),c.half);
      if(remaining>braking && remaining>step)desired=add(step,i.velocity);
      else if(remaining<0.f)desired=i.velocity;
      else {
        // FLD/FMUL/FADD/FSQRT/FSTP with the original 53-bit x87 precision.
        const double product=wide_mul(step,remaining);
        desired=float(std::sqrt(wide_add(product,product)));
      }
    }
    const float minimum=i.allow_reverse?c.reverse_minimum:i.velocity;
    if(minimum>desired)desired=minimum;
    else if(desired>i.velocity_limit)desired=i.velocity_limit;
  }else {
    desired=add(step,i.velocity);
    const float minimum=sub(0.f,i.velocity_limit);
    if(minimum>desired)desired=minimum;
    else {
      const float maximum=i.allow_reverse?c.reverse_maximum:i.velocity;
      if(desired>maximum)desired=maximum;
    }
  }
  const float ratio=absolute(i.velocity_limit)>c.epsilon?
      absolute(div(i.velocity,i.velocity_limit)):0.f;
  float delta=sub(desired,i.velocity);
  const float minimum_delta=sub(0.f,maximum_delta);
  if(minimum_delta>delta)delta=minimum_delta;
  else if(delta>maximum_delta)delta=maximum_delta;
  return {delta,ratio};
}
}
