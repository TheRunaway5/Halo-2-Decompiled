#include "halo2/sound_sdk_spatial.hpp"
#include "halo2/sound_sdk_hrtf.hpp"
#include "halo2/sound_sdk_calculate.hpp"
#include "halo2/sound_sdk_voice_data.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {namespace {
[[noreturn]] void unavailable(){throw std::runtime_error("Original x87/SSE SDK spatial numerical backend unavailable");}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
float real(std::uint32_t word){return std::bit_cast<float>(word);}
std::uint32_t bits(float value){return std::bit_cast<std::uint32_t>(value);}
struct FloatingPointScope {
  std::fenv_t saved;unsigned mxcsr;
  explicit FloatingPointScope(const SoundDriverFloatingPointContext &context):mxcsr(_mm_getcsr()){
    if((context.x87_control_word&0x3fu)!=0x3fu||(context.mxcsr&0x1f80u)!=0x1f80u||(context.mxcsr&0xffff0000u))
      throw std::runtime_error("Unmasked source SDK floating-point exception delivery unavailable");
    if(std::fegetenv(&saved)!=0)unavailable();
    _mm_setcsr(context.mxcsr);asm volatile("fldcw %0"::"m"(context.x87_control_word):"memory");
  }
  ~FloatingPointScope(){std::fesetenv(&saved);_mm_setcsr(mxcsr);}
};
long double dot(const std::array<std::uint32_t,3> &basis,const std::array<std::uint32_t,3> &point){
  long double result;
  asm volatile("flds 8(%1)\n\tfmuls 8(%2)\n\tflds 4(%1)\n\tfmuls 4(%2)\n\tfaddp\n\tflds (%2)\n\tfmuls (%1)\n\tfaddp\n\tfstpt %0"
    :"=m"(result):"r"(basis.data()),"r"(point.data()):"st","st(1)");
  return result;
}
unsigned compare(long double left,long double right){
  unsigned short status;
  asm volatile("fldt %2\n\tfldt %1\n\tfucompp\n\tfnstsw %%ax"
    :"=a"(status):"m"(left),"m"(right):"st","st(1)");
  return status&0x4500u;
}
bool equal(long double a,long double b){return compare(a,b)==0x4000u;}
bool less(long double a,long double b){return compare(a,b)==0x100u;}
bool greater(long double a,long double b){return compare(a,b)==0;}
long double vector_length(const std::array<std::uint32_t,3> &point){
  long double result;
  // Original37DAE0 leaves its extended result in ST0. Preserve the exact
  // x*x+y*y+z*z sequence, independent of the platform's math library.
  asm volatile("flds 8(%1)\n\tflds 4(%1)\n\tflds (%1)\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp\n\tfld %%st(3)\n\tfmul %%st(4),%%st\n\tfaddp\n\tfsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstpt %0"
    :"=m"(result):"r"(point.data()):"st","st(1)","st(2)","st(3)","st(4)");
  return result;
}
std::uint32_t ratio_angle(long double numerator,long double denominator,float scale,float quarter,bool complement){
  float result;
  if(complement){
    // Source37DFD5 subtracts before the float spill.
    asm volatile("fldt %2\n\tfldt %1\n\tfdiv %%st(1),%%st\n\tfmuls %3\n\tfsubrs %4\n\tfstps %0\n\tfstp %%st(0)"
      :"=m"(result):"m"(numerator),"m"(denominator),"m"(scale),"m"(quarter):"st","st(1)");
  }else asm volatile("fldt %2\n\tfldt %1\n\tfdiv %%st(1),%%st\n\tfmuls %3\n\tfstps %0\n\tfstp %%st(0)"
    :"=m"(result):"m"(numerator),"m"(denominator),"m"(scale):"st","st(1)");
  return bits(result);
}
std::int32_t sdk_volume(float value,const SoundSdkSpatialConstants &c){
  const auto low=compare(value,real(c.zero_bits));
  if(low==0x100u||low==0x4000u)return -10000;
  const auto high=compare(value,real(c.one_bits));
  if(high==0||high==0x4000u)return 0;
  const float scale=real(c.volume_scale_bits);float result;
  //37DB3E spills to float before379D4B's signed32 SSE conversion. Neither
  // the game's2000 scale nor32909C's x87 low-word conversion applies here.
  asm volatile("flds %1\n\tfldlg2\n\tfxch %%st(1)\n\tfyl2x\n\tfmuls %2\n\tfstps %0"
    :"=m"(result):"m"(value),"m"(scale):"st","st(1)");
  return _mm_cvttss_si32(_mm_set_ss(result));
}
long double unsigned_real(std::uint32_t value,const SoundSdkVoiceConstants &c){
  long double result;
  if(value&0x80000000u)asm volatile("fildl %1\n\tfadds %2\n\tfstpt %0"
    :"=m"(result):"m"(value),"m"(c.unsigned_correction_bits):"st");
  else asm volatile("fildl %1\n\tfstpt %0":"=m"(result):"m"(value):"st");
  return result;
}
std::uint32_t curve_index(long double value){
  alignas(16) std::array<std::uint32_t,8> scratch{};std::uint32_t result;
  // Complete322B0C low-word result. Preserve FISTP64 and its source carry/borrow
  // corrections; a C++ float-to-unsigned cast is neither equivalent nor defined
  // for all declared nonfinite inputs. No caller FPU operand remains afterward.
  asm volatile("fldt %1\n\tfld %%st(0)\n\tfsts 24(%2)\n\tfistpq 16(%2)\n\tfildq 16(%2)\n\tmovl 24(%2),%%edx\n\tmovl 16(%2),%%eax\n\ttestl %%eax,%%eax\n\tjz 1f\n\t"
    "2: .byte 0xde,0xe9\n\ttestl %%edx,%%edx\n\tjns 3f\n\tfstps (%2)\n\tmovl (%2),%%ecx\n\txorl $0x80000000,%%ecx\n\taddl $0x7fffffff,%%ecx\n\tadcl $0,%%eax\n\tjmp 4f\n\t"
    "3: fstps (%2)\n\tmovl (%2),%%ecx\n\taddl $0x7fffffff,%%ecx\n\tsbbl $0,%%eax\n\tjmp 4f\n\t"
    "1: movl 20(%2),%%edx\n\ttestl $0x7fffffff,%%edx\n\tjnz 2b\n\tfstps 24(%2)\n\tfstps 24(%2)\n\t4:"
    :"=&a"(result):"m"(value),"r"(scratch.data()):"memory","edx","ecx","cc","st","st(1)");
  return result;
}
std::uint32_t truncate_word(std::uint32_t value){
  return std::bit_cast<std::uint32_t>(_mm_cvttss_si32(_mm_set_ss(real(value))));
}
std::uint32_t voice_log(std::uint32_t value,std::uint32_t scale){
  std::uint32_t result;
  asm volatile("flds %1\n\tfldlg2\n\tfxch %%st(1)\n\tfyl2x\n\tfmuls %2\n\tfstps %0"
    :"=m"(result):"m"(value),"m"(scale):"st","st(1)");
  return truncate_word(result);
}
std::uint32_t voice_gain(std::uint32_t value,const SoundSdkVoiceConstants &c){
  if(compare(real(value),real(c.zero_bits))&0x100u)return 0;
  std::uint32_t result;
  asm volatile("flds %1\n\tfadds %2\n\tfldlg2\n\tfxch %%st(1)\n\tfyl2x\n\tfmuls %3\n\tfstps %0"
    :"=m"(result):"m"(value),"m"(c.one_bits),"m"(c.gain_scale_bits):"st","st(1)");
  return truncate_word(result);
}
std::uint32_t voice_volume(std::uint32_t value,const SoundSdkVoiceConstants &c){
  if(less(real(value),real(c.zero_bits))||equal(real(value),real(c.zero_bits)))return std::uint32_t(-10000);
  if(!(compare(real(value),real(c.one_bits))&0x100u))return 0;
  return voice_log(value,c.volume_scale_bits);
}
std::uint32_t db_exponent(std::uint32_t level,const SoundSdkVoiceConstants &c){
  std::uint32_t scaled,rounded,result,discard;
  //383167's original immediate float40549A78 and FLD1 are executable literals.
  const std::uint32_t logarithm=0x40549a78u,one=0x3f800000u;
  asm volatile("fildl %4\n\tfmuls %5\n\tfstps %1\n\tflds %6\n\tfmuls %1\n\tfrndint\n\tfsts %2\n\tfld1\n\tfscale\n\tfxch %%st(1)\n\tfstps %3\n\tflds %6\n\tfmuls %1\n\tfsubs %2\n\tf2xm1\n\tfadds %7\n\tfmulp\n\tfstps %0"
    :"=m"(result),"=m"(scaled),"=m"(rounded),"=m"(discard)
    :"m"(level),"m"(c.db_exponent_scale_bits),"m"(logarithm),"m"(one):"st","st(1)");
  return result;
}
std::uint32_t environment_filter(std::uint32_t first,std::uint32_t level,
    std::uint32_t frequency,std::uint32_t sample_rate,const SoundSdkVoiceConstants &c){
  if(std::bit_cast<std::int32_t>(first)<=-10000||!level)return 0;
  double cosine;
  asm volatile("flds %1\n\tfdivs %2\n\tfmuls %3\n\tfcos\n\tfstpl %0"
    :"=m"(cosine):"m"(frequency),"m"(sample_rate),"m"(c.two_pi_bits):"st");
  const auto gain=db_exponent(level*2u,c);long double coefficient;
  asm volatile("flds %2\n\tfldl %1\n\tfmul %%st(1),%%st\n\tfsubrl %3\n\tfld1\n\tfsubl %1\n\tfmul %%st(2),%%st\n\tfadd %%st(0),%%st\n\tfldl %1\n\tfmull %1\n\tfsubrl %3\n\tfmul %%st(3),%%st\n\tfmul %%st(3),%%st\n\t.byte 0xde,0xe9\n\tfsqrt\n\t.byte 0xde,0xe9\n\tfld1\n\tfsub %%st(2),%%st\n\t.byte 0xde,0xf9\n\tfstp %%st(1)\n\tfstpt %0"
    :"=m"(coefficient):"m"(cosine),"m"(gain),"m"(c.one_double_bits):"st","st(1)","st(2)","st(3)");
  if(!(compare(coefficient,real(c.one_bits))&0x100u))return 0xffffu;
  std::uint32_t scaled;
  asm volatile("fldt %1\n\tfmuls %2\n\tfstps %0":"=m"(scaled):"m"(coefficient),"m"(c.filter_scale_bits):"st");
  return truncate_word(scaled);
}
std::uint32_t clamp_volume(std::uint32_t value){
  const auto signed_value=std::bit_cast<std::int32_t>(value);
  return signed_value<-10000?std::uint32_t(-10000):signed_value>0?0u:value;
}
#endif
} // namespace

SoundSdkPosition source_sound_sdk_position(std::uint32_t mode,
    const std::array<std::uint32_t,3> &listener,const std::array<std::uint32_t,3> &source,
    const SoundSdkSpatialConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);SoundSdkPosition result{};
  asm volatile("flds %1\n\tfstps %0":"=m"(result.direction[0]):"m"(source[0]):"st");
  result.direction[2]=source[2];
  if(mode!=1){
    for(unsigned axis:{0u,2u})asm volatile("flds %1\n\tfsubs %2\n\tfstps %0"
      :"=m"(result.direction[axis]):"m"(result.direction[axis]),"m"(listener[axis]):"st");
  }
  const auto x=real(result.direction[0]),z=real(result.direction[2]),zero=real(c.zero_bits),sentinel=real(c.position_sentinel_bits);
  if(equal(x,zero)&&equal(z,zero)){result.distance_bits=c.zero_bits;return result;}
  if(equal(x,sentinel)||equal(z,sentinel)){result.distance_bits=c.position_sentinel_bits;return result;}
  auto *point=result.direction.data();
  //380168 uses reciprocal then multiply, with original x87 operation order.
  // It explicitly clears y, and treats positive FLT_MAX as a sentinel.
  asm volatile("flds 8(%1)\n\tflds (%1)\n\tfld %%st(1)\n\tfmul %%st(2),%%st\n\tfld %%st(1)\n\tfmul %%st(2),%%st\n\tfaddp\n\tfsqrt\n\tfstp %%st(2)\n\tfstp %%st(0)\n\tfld1\n\tfdiv %%st(1),%%st\n\tfld %%st(0)\n\tfmuls (%1)\n\tfstps (%1)\n\tfmuls 8(%1)\n\tfstps 8(%1)\n\tfstps %0"
    :"=m"(result.distance_bits):"r"(point):"memory","st","st(1)","st(2)","st(3)");
  return result;
#else
  (void)mode;(void)listener;(void)source;(void)c;(void)fp;unavailable();
#endif
}

SoundSdkAngles source_sound_sdk_angles(std::uint32_t mode,
    const std::array<std::uint32_t,3> &forward,const std::array<std::uint32_t,3> &side,
    const std::array<std::uint32_t,3> &point,std::uint32_t distance,
    const SoundSdkSpatialConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);
  const auto front=dot(mode==1?c.mode_one_forward:forward,point);
  const auto right_extended=dot(mode==1?c.mode_one_side:side,point);
  float right;
  asm volatile("fldt %1\n\tfstps %0":"=m"(right):"m"(right_extended):"st");
  const auto zero=real(c.zero_bits);SoundSdkAngles result{};
  if(!equal(real(distance),zero)){
    const auto absolute_front=std::fabs(front),absolute_side=std::fabs(static_cast<long double>(right));
    if(greater(absolute_front,absolute_side))
      result.azimuth_bits=ratio_angle(absolute_side,absolute_front,real(c.octant_bits),real(c.quarter_turn_bits),false);
    else if(!equal(absolute_side,zero))
      result.azimuth_bits=ratio_angle(absolute_front,absolute_side,real(c.octant_bits),real(c.quarter_turn_bits),true);
  }
  if(less(front,zero)){
    asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(result.azimuth_bits):"m"(c.half_turn_bits),"m"(result.azimuth_bits):"st");
  }
  if(less(right,zero))asm volatile("flds %1\n\tfchs\n\tfstps %0":"=m"(result.azimuth_bits):"m"(result.azimuth_bits):"st");
  return result;
#else
  (void)mode;(void)forward;(void)side;(void)point;(void)distance;(void)c;(void)fp;unavailable();
#endif
}

SoundSdkFrontRear source_sound_sdk_front_rear(std::uint32_t distance,std::uint32_t scale,
    std::uint32_t azimuth,std::uint32_t enabled,const SoundSdkSpatialConstants &c,
    const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);if(!enabled)return {0,-10000};
  long double product,alpha_extended;float angle,alpha;
  asm volatile("flds %1\n\tfmuls %2\n\tfstpt %0":"=m"(product):"m"(distance),"m"(scale):"st");
  asm volatile("flds %1\n\tfabs\n\tfstps %0":"=m"(angle):"m"(azimuth):"st");
  asm volatile("flds %3\n\tflds %4\n\tflds %2\n\tfmuls %5\n\tfsub %%st(1),%%st\n\tfadd %%st(1),%%st\n\tfmul %%st(2),%%st\n\tfadd %%st(0),%%st\n\tfsub %%st(2),%%st\n\tfsts %0\n\tfstpt %1\n\tfstp %%st(0)\n\tfstp %%st(0)"
    :"=m"(alpha),"=m"(alpha_extended):"m"(angle),"m"(c.half_bits),"m"(c.one_bits),"m"(c.azimuth_scale_bits):"st","st(1)","st(2)");
  if(greater(alpha_extended,real(c.one_bits)))alpha=real(c.one_bits);
  else if(less(alpha,real(c.zero_bits)))alpha=0.f;
  if(less(product,real(c.half_bits))){
    asm volatile("fldt %1\n\tflds %2\n\tflds %3\n\tflds %4\n\tfsub %%st(2),%%st\n\tfmul %%st(3),%%st\n\tfadd %%st(1),%%st\n\tfmul %%st(2),%%st\n\tfstps %0\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstp %%st(0)"
      :"=m"(alpha):"m"(product),"m"(c.half_bits),"m"(c.one_bits),"m"(alpha):"st","st(1)","st(2)","st(3)");
  }
  float front;
  asm volatile("fld1\n\tfsubs %1\n\tfstps %0":"=m"(front):"m"(alpha):"st");
  return {sdk_volume(front,c),sdk_volume(alpha,c)};
#else
  (void)distance;(void)scale;(void)azimuth;(void)enabled;(void)c;(void)fp;unavailable();
#endif
}

std::array<std::uint32_t,2> source_sound_sdk_kernel(std::uint32_t azimuth,
    std::uint32_t fold,const SoundSdkSpatialConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);float rounded;
  asm volatile("flds %1\n\tfabs\n\tfadds %2\n\tfstps %0":"=m"(rounded):"m"(azimuth),"m"(c.angle_round_bias_bits):"st");
  const auto divided=_mm_cvttss_si32(_mm_set_ss(rounded))/3;
  auto angle=std::uint32_t(divided)*3u;
  if(fold&&std::bit_cast<std::int32_t>(angle)>90)angle=180u-angle;
  if(std::bit_cast<std::int32_t>(angle)>180)angle=0;
  const auto index=std::bit_cast<std::int32_t>(180u-angle)/3;
  const auto offset=std::uint32_t(index)<<6;
  const auto a=c.kernel_table_a+offset,b=c.kernel_table_b+offset;
  // Original x87 C0 is also set for an unordered comparison.
  return compare(real(azimuth),real(c.zero_bits))&0x100u?std::array{b,a}:std::array{a,b};
#else
  (void)azimuth;(void)fold;(void)c;(void)fp;unavailable();
#endif
}

std::array<std::uint32_t,3> source_sound_sdk_cross(const std::array<std::uint32_t,3> &a,
    const std::array<std::uint32_t,3> &b,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);std::array<std::uint32_t,3> result;
  // The DE E9 bytes are the original FSUBP ST1 instruction. Explicit encoding
  // avoids the assembler's AT&T/Intel register-subtraction spelling ambiguity.
  asm volatile("flds 4(%2)\n\tfmuls 8(%1)\n\tflds 8(%2)\n\tfmuls 4(%1)\n\t.byte 0xde,0xe9\n\tfstps (%0)\n\tflds (%1)\n\tfmuls 8(%2)\n\tflds (%2)\n\tfmuls 8(%1)\n\t.byte 0xde,0xe9\n\tfstps 4(%0)\n\tflds (%2)\n\tfmuls 4(%1)\n\tflds (%1)\n\tfmuls 4(%2)\n\t.byte 0xde,0xe9\n\tfstps 8(%0)"
    ::"r"(result.data()),"r"(a.data()),"r"(b.data()):"memory","st","st(1)");
  return result;
#else
  (void)a;(void)b;(void)fp;unavailable();
#endif
}

std::uint32_t source_sound_sdk_cone(const std::array<std::uint32_t,3> &a,
    const std::array<std::uint32_t,3> &b,const SoundSdkConeConstants &c,
    const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);std::array<std::uint32_t,3> difference,sum;
  for(unsigned i=0;i<3;++i)asm volatile("flds %1\n\tfsubs %2\n\tfstps %0"
    :"=m"(difference[i]):"m"(a[i]),"m"(b[i]):"st");
  const auto extended_difference=vector_length(difference);float first;
  asm volatile("fldt %1\n\tfstps %0":"=m"(first):"m"(extended_difference):"st");
  asm volatile("flds %1\n\tfadds %2\n\tfstps %0":"=m"(sum[0]):"m"(a[0]),"m"(b[0]):"st");
  for(unsigned i:{1u,2u})asm volatile("flds %1\n\tfadds %2\n\tfstps %0"
    :"=m"(sum[i]):"m"(b[i]),"m"(a[i]):"st");
  const auto second=vector_length(sum);std::uint32_t result;
  if(less(second,first))asm volatile("fldt %1\n\tfdivs %2\n\tfmuls %3\n\tfmuls %4\n\tfstps %0"
    :"=m"(result):"m"(second),"m"(first),"m"(c.octant_bits),"m"(c.angle_scale_bits):"st");
  else asm volatile("fldt %1\n\tflds %2\n\tfdiv %%st(1),%%st\n\tfmuls %3\n\tfsubrs %4\n\tfmuls %5\n\tfstps %0\n\tfstp %%st(0)"
    :"=m"(result):"m"(second),"m"(first),"m"(c.octant_bits),"m"(c.quarter_turn_bits),"m"(c.angle_scale_bits):"st","st(1)");
  return result;
#else
  (void)a;(void)b;(void)c;(void)fp;unavailable();
#endif
}

std::uint32_t source_sound_sdk_velocity(std::uint32_t mode,
    const std::array<std::uint32_t,3> &listener,const std::array<std::uint32_t,3> &source,
    const std::array<std::uint32_t,3> &direction,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);std::uint32_t result;
  if(mode==1)asm volatile("flds 8(%1)\n\tfmuls 8(%2)\n\tflds 4(%1)\n\tfmuls 4(%2)\n\tfaddp\n\tflds (%1)\n\tfmuls (%2)\n\tfaddp\n\tfstps %0"
    :"=m"(result):"r"(source.data()),"r"(direction.data()):"st","st(1)");
  else asm volatile("flds 8(%1)\n\tfsubs 8(%2)\n\tfmuls 8(%3)\n\tflds 4(%1)\n\tfsubs 4(%2)\n\tfmuls 4(%3)\n\tfaddp\n\tflds (%1)\n\tfsubs (%2)\n\tfmuls (%3)\n\tfaddp\n\tfstps %0"
    :"=m"(result):"r"(source.data()),"r"(listener.data()),"r"(direction.data()):"st","st(1)");
  return result;
#else
  (void)mode;(void)listener;(void)source;(void)direction;(void)fp;unavailable();
#endif
}

std::uint32_t source_sound_sdk_gain(std::uint32_t value,const SoundSdkVoiceConstants &c,
    const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);return voice_gain(value,c);
#else
  (void)value;(void)c;(void)fp;unavailable();
#endif
}
std::uint32_t source_sound_sdk_volume(std::uint32_t value,const SoundSdkVoiceConstants &c,
    const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);return voice_volume(value,c);
#else
  (void)value;(void)c;(void)fp;unavailable();
#endif
}
std::uint32_t source_sound_sdk_distance(const SoundSdkDistanceParameters &p,
    const SoundSdkVoiceConstants &c,const SoundDriverFloatingPointContext &fp,
    const SoundSdkCurveReader &curve){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);
  if(!greater(real(p.distance_bits),real(p.min_distance_bits)))return 0;
  if(p.clip_at_maximum&&!(compare(real(p.distance_bits),real(p.max_distance_bits))&0x100u))return std::uint32_t(-10000);
  auto distance=p.distance_bits;
  if(greater(real(distance),real(p.max_distance_bits)))distance=p.max_distance_bits;
  if(p.curve_identity&&p.curve_count){
    if(!curve)throw std::runtime_error("Original SDK distance curve owner unavailable");
    const auto count=unsigned_real(p.curve_count,c);long double step,offset,index_value;std::uint32_t inverse;
    asm volatile("flds %4\n\tfsubs %5\n\tflds %6\n\tfsubs %5\n\tfldt %7\n\t.byte 0xde,0xf9\n\tfld1\n\tfdiv %%st(1),%%st\n\tfsts %0\n\tfmul %%st(2),%%st\n\tfstpt %1\n\tfstpt %2\n\tfstpt %3"
      :"=m"(inverse),"=m"(index_value),"=m"(step),"=m"(offset)
      :"m"(distance),"m"(p.min_distance_bits),"m"(p.max_distance_bits),"m"(count):"st","st(1)","st(2)");
    auto index=curve_index(index_value);if(index>=p.curve_count)index=p.curve_count-1u;
    std::uint32_t previous=0x3f800000u,current;long double remainder;
    if(!index){current=curve(p.curve_identity);remainder=offset;}
    else{
      const auto identity=p.curve_identity+index*4u;
      previous=curve(identity-4u);current=curve(identity);
      const auto index_real=unsigned_real(index,c);
      asm volatile("fldt %1\n\tfldt %2\n\tfldt %3\n\tfmul %%st(1),%%st\n\t.byte 0xde,0xea\n\tfstp %%st(0)\n\tfstpt %0"
        :"=m"(remainder):"m"(offset),"m"(step),"m"(index_real):"st","st(1)","st(2)");
    }
    std::uint32_t interpolated;
    asm volatile("fldt %1\n\tflds %2\n\tfmul %%st(1),%%st\n\tflds %3\n\tfsubs %4\n\tfmulp\n\tfadds %4\n\tfstps %0\n\tfstp %%st(0)"
      :"=m"(interpolated):"m"(remainder),"m"(inverse),"m"(current),"m"(previous):"st","st(1)","st(2)");
    return voice_volume(interpolated,c);
  }
  std::uint32_t attenuation;
  asm volatile("flds %1\n\tfdivs %2\n\tfsubs %3\n\tfmuls %4\n\tfstps %0"
    :"=m"(attenuation):"m"(distance),"m"(p.min_distance_bits),"m"(c.one_bits),"m"(p.rolloff_bits):"st");
  return voice_gain(attenuation,c);
#else
  (void)p;(void)c;(void)fp;(void)curve;unavailable();
#endif
}
std::uint32_t source_sound_sdk_cone_volume(std::uint32_t inner,std::uint32_t outer,
    std::uint32_t volume,std::uint32_t angle,const SoundSdkVoiceConstants &c,
    const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);const auto first=unsigned_real(inner,c);
  if(less(real(angle),first)||equal(real(angle),first))return 0;
  if(inner<outer){const auto last=unsigned_real(outer,c);
    if(less(last,real(angle))||equal(last,real(angle)))return volume;}
  auto difference=outer-inner;if(!difference)difference=1;
  const auto width=unsigned_real(difference,c);std::uint32_t result;
  asm volatile("fldt %1\n\tflds %2\n\tfsub %%st(1),%%st\n\tfimull %3\n\tfldt %4\n\t.byte 0xde,0xf9\n\tfstps %0\n\tfstp %%st(0)"
    :"=m"(result):"m"(first),"m"(angle),"m"(volume),"m"(width):"st","st(1)","st(2)");
  return truncate_word(result);
#else
  (void)inner;(void)outer;(void)volume;(void)angle;(void)c;(void)fp;unavailable();
#endif
}
std::uint32_t source_sound_sdk_doppler(std::uint32_t factor,std::uint32_t scale,
    std::uint32_t velocity,const SoundSdkVoiceConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);long double product;
  asm volatile("flds %1\n\tfmuls %2\n\tfmuls %3\n\tfstpt %0"
    :"=m"(product):"m"(factor),"m"(scale),"m"(velocity):"st");
  if(equal(product,real(c.zero_bits)))return 0;
  if(!(compare(product,real(c.doppler_max_bits))&0x100u))return std::uint32_t(-32767);
  if(less(product,real(c.doppler_min_bits))||equal(product,real(c.doppler_min_bits)))return 4096;
  std::uint32_t logarithm,result;const std::uint32_t pitch_scale=0x45800000u;
  asm volatile("fldt %1\n\tfmuls %2\n\tfsubrs %3\n\tfstps %0"
    :"=m"(logarithm):"m"(product),"m"(c.doppler_reciprocal_bits),"m"(c.one_bits):"st");
  //3801DF's immediate4096 scale and FISTP32 obey the caller's x87 rounding.
  asm volatile("flds %1\n\tflds %2\n\tfyl2x\n\tfistpl %0"
    :"=m"(result):"m"(pitch_scale),"m"(logarithm):"st","st(1)");
  return result;
#else
  (void)factor;(void)scale;(void)velocity;(void)c;(void)fp;unavailable();
#endif
}
std::array<std::uint32_t,2> source_sound_sdk_rear_widen(std::uint32_t,std::uint32_t){
  // Complete37E112, selected by the original LightHRTF4 initializer.
  return {0,std::uint32_t(-10000)};
}
std::array<std::uint32_t,4> source_sound_sdk_environment(std::uint32_t distance,
    std::uint32_t rolloff,std::uint32_t frequency,const std::array<std::uint32_t,10> &s,
    const SoundSdkVoiceConstants &c,const SoundDriverFloatingPointContext &fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  FloatingPointScope scope(fp);std::uint32_t product,high,complement_high,sum,complement_sum;
  asm volatile("flds %1\n\tfmuls %2\n\tfmuls %3\n\tfstps %0"
    :"=m"(product):"m"(distance),"m"(s[5]),"m"(rolloff):"st");
  asm volatile("fildl %2\n\tfmuls %3\n\tfildl %4\n\tfmuls %5\n\tfsts %0\n\tfadd %%st(1),%%st\n\tfstps %1\n\tfstp %%st(0)"
    :"=m"(high),"=m"(sum):"m"(s[6]),"m"(s[7]),"m"(s[8]),"m"(s[9]):"st","st(1)");
  const auto direct=truncate_word(sum)+s[1];
  asm volatile("fld1\n\tfsubs %3\n\tfimull %2\n\tfld1\n\tfsubs %5\n\tfimull %4\n\tfsts %0\n\tfadd %%st(1),%%st\n\tfstps %1\n\tfstp %%st(0)"
    :"=m"(complement_high),"=m"(complement_sum):"m"(s[6]),"m"(s[7]),"m"(s[8]),"m"(s[9]):"st","st(1)");
  const auto direct_level=clamp_volume(truncate_word(complement_sum)+s[2]);
  auto attenuated_high=high;
  if(greater(real(product),real(c.one_bits))){
    const auto attenuation=(less(real(product),real(c.environment_log_floor_bits))||equal(real(product),real(c.environment_log_floor_bits)))?
        std::uint32_t(-10000):voice_log(product,c.volume_scale_bits);
    asm volatile("fildl %1\n\tfadds %2\n\tfstps %0":"=m"(attenuated_high):"m"(attenuation),"m"(high):"st");
  }
  const auto room=truncate_word(attenuated_high)+s[3];
  const auto room_level=clamp_volume(truncate_word(complement_high)+s[3]);
  const auto first_filter=environment_filter(0,direct_level,frequency,c.sample_rate_bits,c);
  const auto second_filter=environment_filter(0,room_level,frequency,c.sample_rate_bits,c);
  return {direct,room,first_filter,second_filter};
#else
  (void)distance;(void)rolloff;(void)frequency;(void)s;(void)c;(void)fp;unavailable();
#endif
}

void source_sound_sdk_calculate(std::uint32_t listener,std::uint32_t source,
    const SoundDriverFloatingPointContext &fp,const SoundSdkCalculateServices &services,
    const SoundDriverStartupObserver &observer){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!services.under_state_lock)throw std::runtime_error("Original SDK retained lock owner unavailable");
  unsigned invocations=0;
  services.under_state_lock([&]{
    if(invocations++)throw std::runtime_error("Original SDK retained lock callback repeated");
    FloatingPointScope scope(fp);
    const auto memory=[&](std::uint32_t address,std::size_t size){
      if(!services.memory)throw std::runtime_error("Original SDK retained memory owner unavailable");
      const auto bytes=services.memory(address,size);
      if(bytes.size()!=size)throw std::runtime_error("Original SDK retained backing extent differs");
      return bytes;
    };
    const auto word=[&](std::uint32_t address){const auto bytes=memory(address,4);std::uint32_t value=0;
      for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
      return value;
    };
    const auto vector=[&](std::uint32_t address){return std::array{word(address),word(address+4u),word(address+8u)};};
    const auto put=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value){
      const auto before=word(address);auto bytes=memory(address,4);
      for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));
      if(observer)observer({pc,address,4,before,value});
    };
    const auto put_vector=[&](const std::array<std::uint32_t,3> &pcs,std::uint32_t address,
                             const std::array<std::uint32_t,3> &value){
      for(unsigned i=0;i<3;++i)put(pcs[i],address+i*4u,value[i]);
    };
    auto changed=word(listener),valid=word(listener+4u);
    if(source){changed|=word(source);valid|=word(source+4u);}
    if((changed&4u)||!(valid&0x40u)){
      const auto side=source_sound_sdk_cross(vector(listener+0x20u),vector(listener+0x2cu),fp);
      if(side!=vector(listener+0x44u)){
        changed|=0x40u;put_vector({0x37ca74,0x37ca75,0x37ca76},listener+0x44u,side);
      }
      valid|=0x40u;
    }
    if(changed&0x400000u){
      if(word(source+0x40u)!=2)changed|=0x07ff0000u;
      else changed=0x400000u;
    }
    if(source&&word(source+0x40u)!=2){
      if((changed&0x410001u)||(valid&0x18000000u)!=0x18000000u){
        if(!services.position)throw std::runtime_error("Original SDK current position owner unavailable");
        const auto position=services.position(word(source+0x40u),vector(listener+8u),vector(source+8u));
        if(position.direction!=vector(source+0x5cu)){
          changed|=0x08000000u;put_vector({0x37caf5,0x37caf6,0x37caf7},source+0x5cu,position.direction);
        }
        if(!equal(real(position.distance_bits),real(word(source+0x68u)))){
          changed|=0x10000000u;put(0x37cb0e,source+0x68u,position.distance_bits);
        }
        valid|=0x18000000u;
      }
      if((changed&0x18400044u)||!(valid&0x20000000u)){
        if(!services.angles)throw std::runtime_error("Original SDK current angle owner unavailable");
        auto distance=word(source+0x68u);
        asm volatile("flds %1\n\tfstps %0":"=m"(distance):"m"(distance):"st");
        const auto angle=services.angles(word(source+0x40u),vector(listener+0x20u),vector(listener+0x2cu),
                                        vector(listener+0x44u),vector(source+0x5cu),distance);
        if(!equal(real(angle.azimuth_bits),real(word(source+0x74u)))||
           !equal(real(angle.elevation_bits),real(word(source+0x78u)))){
          changed|=0x20000000u;put(0x37cb76,source+0x74u,angle.azimuth_bits);put(0x37cb7c,source+0x78u,angle.elevation_bits);
        }
        valid|=0x20000000u;
      }
      if((changed&0x08080000u)||!(valid&0x40000000u)){
        if(!services.cone)throw std::runtime_error("Original SDK current cone owner unavailable");
        const auto cone=services.cone(vector(source+0x28u),vector(source+0x5cu));
        if(!equal(real(cone),real(word(source+0x70u)))){
          changed|=0x40000000u;put(0x37cbbb,source+0x70u,cone);
        }
        valid|=0x40000000u;
      }
      if((changed&0x08420002u)||!(valid&0x80000000u)){
        if(!services.velocity)throw std::runtime_error("Original SDK current velocity owner unavailable");
        const auto velocity=services.velocity(word(source+0x40u),vector(listener+0x14u),vector(source+0x14u),vector(source+0x5cu));
        if(!equal(real(velocity),real(word(source+0x6cu)))){
          changed|=0x80000000u;put(0x37cc06,source+0x6cu,velocity);
        }
        valid|=0x80000000u;
      }
    }
    if(changed&0x7fu)put(0x37cc17,listener,changed&0x7fu);
    if(valid&0x7fu)put(0x37cc21,listener+4u,valid&0x7fu);
    if(changed&0xffff0000u)put(0x37cc2d,source,changed&0xffff0000u);
    if(valid&0xffff0000u)put(0x37cc36,source+4u,valid&0xffff0000u);
  });
  if(invocations!=1)throw std::runtime_error("Original SDK retained lock callback not executed");
#else
  (void)listener;(void)source;(void)fp;(void)services;(void)observer;unavailable();
#endif
}

void source_sound_sdk_voice_data(std::uint32_t listener,std::uint32_t source,
    std::uint32_t listener_environment,std::uint32_t source_environment,
    std::uint32_t voice,const SoundSdkVoiceConstants &c,const SoundDriverFloatingPointContext &fp,
    const SoundSdkVoiceDataServices &services,const SoundDriverStartupObserver &observer){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!services.under_state_lock)throw std::runtime_error("Original SDK voice-data state lock unavailable");
  unsigned invocations=0;
  services.under_state_lock([&]{
    if(invocations++)throw std::runtime_error("Original SDK voice-data lock callback repeated");
    FloatingPointScope scope(fp);
    const auto memory=[&](std::uint32_t address,std::size_t size){
      if(!services.memory)throw std::runtime_error("Original SDK voice-data memory owner unavailable");
      const auto bytes=services.memory(address,size);
      if(bytes.size()!=size)throw std::runtime_error("Original SDK voice-data backing extent differs");
      return bytes;
    };
    const auto word=[&](std::uint32_t address){
      const auto bytes=memory(address,4);std::uint32_t value=0;
      for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
      return value;
    };
    const auto put=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned size=4){
      auto bytes=memory(address,size);std::uint32_t before=0;
      for(unsigned i=0;i<size;++i){before|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);bytes[i]=std::byte(value>>(i*8));}
      if(observer)observer({pc,address,size,before,value});
    };
    const auto flag=[&](std::uint32_t pc,std::uint32_t bits,unsigned size=4){
      const auto before=size==4?word(voice):std::to_integer<std::uint32_t>(memory(voice,1)[0]);
      put(pc,voice,before|bits,size);
    };
    const auto load=[&](std::uint32_t address){
      const auto input=word(address);std::uint32_t value;
      asm volatile("flds %1\n\tfstps %0":"=m"(value):"m"(input):"st");return value;
    };
    const auto multiply=[&](std::uint32_t first,std::uint32_t second){
      const auto a=word(first),b=word(second);std::uint32_t result;
      asm volatile("flds %1\n\tfmuls %2\n\tfstps %0":"=m"(result):"m"(a),"m"(b):"st");return result;
    };
    auto changed=word(listener)|word(source);if(changed&0x400000u)changed|=0xffff0000u;
    if(changed&0x15200010u){
      std::uint32_t value=0;
      if(word(source+0x40u)!=2){
        if(!services.distance)throw std::runtime_error("Original SDK current distance algorithm unavailable");
        const auto distance=load(source+0x68u),maximum=load(source+0x3cu),minimum=load(source+0x38u);
        value=services.distance({multiply(listener+0x3cu,source+0x48u),minimum,maximum,
            word(source+0x50u),word(source+0x54u),distance,(word(source+0x58u)>>2)&1u});
      }
      if(value!=word(voice+4u)){flag(0x37aedd,1);put(0x37aee0,voice+4u,value);}
    }
    if(changed&0x40140000u){
      std::uint32_t value=0;
      if(word(source+0x40u)!=2){
        if(!services.cone_volume)throw std::runtime_error("Original SDK current cone-volume algorithm unavailable");
        value=services.cone_volume(word(source+0x20u),word(source+0x24u),word(source+0x34u),load(source+0x70u));
      }
      if(value!=word(voice+8u)){flag(0x37af19,2);put(0x37af1c,voice+8u,value);}
    }
    if(changed&0x34800008u){
      std::array<std::uint32_t,2> value{};
      if(word(source+0x40u)!=2){
        if(!services.front_rear)throw std::runtime_error("Original SDK current front/rear algorithm unavailable");
        const auto elevation=load(source+0x78u),azimuth=load(source+0x74u),distance=load(source+0x68u);
        const auto pair=services.front_rear(multiply(listener+0x38u,source+0x44u),distance,azimuth,elevation,word(source+0x58u)&1u);
        value={std::bit_cast<std::uint32_t>(pair.front),std::bit_cast<std::uint32_t>(pair.rear)};
      }
      if(value[0]!=word(voice+0xcu)||value[1]!=word(voice+0x10u)){
        flag(0x37af7f,4);put(0x37af82,voice+0xcu,value[0]);put(0x37af85,voice+0x10u,value[1]);
      }
    }
    if(changed&0x24000000u){
      std::array<std::uint32_t,2> value{};
      if(word(source+0x40u)!=2&&(word(source+0x58u)&2u)){
        if(!services.rear_widen)throw std::runtime_error("Original SDK current rear-widening algorithm unavailable");
        const auto elevation=load(source+0x78u),azimuth=load(source+0x74u);
        value=services.rear_widen(azimuth,elevation);
      }
      if(value[0]!=word(voice+0x14u)||value[1]!=word(voice+0x18u)){
        flag(0x37afd2,8);put(0x37afd5,voice+0x14u,value[0]);put(0x37afd8,voice+0x18u,value[1]);
      }
    }
    if(changed&0x82800028u){
      std::uint32_t value=0;
      if(word(source+0x40u)!=2){
        if(!services.doppler)throw std::runtime_error("Original SDK current Doppler algorithm unavailable");
        const auto velocity=load(source+0x6cu),scale=multiply(listener+0x40u,source+0x4cu),factor=multiply(listener+0x38u,source+0x44u);
        value=services.doppler(factor,scale,velocity);
      }
      if(value!=word(voice+0x24u)){flag(0x37b01f,0x20);put(0x37b022,voice+0x24u,value);}
    }
    if(changed&0x20000000u){
      std::uint32_t azimuth=0,elevation=0;
      if(word(source+0x40u)!=2){azimuth=load(source+0x74u);elevation=word(source+0x78u);}
      if(!equal(real(azimuth),real(word(voice+0x28u)))||!equal(real(elevation),real(word(voice+0x2cu)))){
        put(0x37b069,voice+0x28u,azimuth);flag(0x37b06c,0x40);put(0x37b06f,voice+0x2cu,elevation);
      }
    }
    if(listener_environment&&source_environment&&((changed&0x14000000u)||
        ((word(listener_environment)|word(source_environment))&0x7f0804u))){
      std::array<std::uint32_t,4> value{};
      if(word(source+0x40u)!=2){
        const auto frequency=load(listener_environment+0x30u),rolloff=load(listener_environment+0xcu),distance=load(source+0x68u);
        std::array<std::uint32_t,10> parameters;
        for(unsigned i=0;i<parameters.size();++i)parameters[i]=word(source_environment+i*4u);
        value=source_sound_sdk_environment(distance,rolloff,frequency,parameters,c,fp);
        if(word(source+0x58u)&2u)value[3]=value[2];
      }
      if(value[0]!=word(voice+0x1cu)||value[1]!=word(voice+0x20u)){
        flag(0x37b101,0x10);put(0x37b104,voice+0x1cu,value[0]);put(0x37b10a,voice+0x20u,value[1]);
      }
      if(value[2]!=word(voice+0x30u)||value[3]!=word(voice+0x34u)){
        flag(0x37b11d,0x80,1);put(0x37b120,voice+0x30u,value[2]);put(0x37b126,voice+0x34u,value[3]);
      }
    }
  });
  if(invocations!=1)throw std::runtime_error("Original SDK voice-data lock callback not executed");
#else
  (void)listener;(void)source;(void)listener_environment;(void)source_environment;
  (void)voice;(void)c;(void)fp;(void)services;(void)observer;unavailable();
#endif
}

void source_sound_sdk_hrtf_apply(std::uint32_t voice,std::uint32_t source_override,
    std::uint32_t parameters_override,const SoundSdkHrtfBindings &bindings,
    const SoundSdkSpatialConstants &constants,const SoundDriverFloatingPointContext &fp,
    const SoundSdkHrtfServices &services,const SoundDriverStartupObserver &observer){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(!services.under_device_lock)throw std::runtime_error("Original HRTF device lock owner unavailable");
  unsigned invocations=0;
  services.under_device_lock([&]{
    if(invocations++)throw std::runtime_error("Original HRTF device lock callback repeated");
    FloatingPointScope scope(fp);
    const auto memory=[&](std::uint32_t address,std::size_t size){
      if(!services.memory)throw std::runtime_error("Original HRTF retained memory owner unavailable");
      const auto value=services.memory(address,size);
      if(value.size()!=size)throw std::runtime_error("Original HRTF retained backing extent differs");
      return value;
    };
    const auto word=[&](std::uint32_t address){
      const auto raw=memory(address,4);std::uint32_t value=0;
      for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
      return value;
    };
    auto source=source_override;
    if(!source){const auto retained=word(word(voice+0x70u)+0xb4u);if(retained)source=retained;}
    auto parameters=parameters_override;
    if(!parameters){const auto retained=word(word(voice+0x70u)+0xb8u);if(retained)parameters=retained;}
    std::array<std::uint32_t,2> kernels{bindings.null_kernel_identity,bindings.null_kernel_identity};
    if(parameters&&(!source||word(source+0x40u)!=2)){
      const auto configuration=word(bindings.speaker_config_address);
      const auto fold=(configuration&0xffffu)==2||(configuration&0x10000u)?1u:0u;
      if(!services.select_kernel)throw std::runtime_error("Original HRTF current algorithm dispatch unavailable");
      const auto load=[&](std::uint32_t address){
        const auto input=word(address);std::uint32_t output;
        asm volatile("flds %1\n\tfstps %0":"=m"(output):"m"(input):"st");return output;
      };
      // Original381354..381368 loads/spills elevation before azimuth.
      const auto elevation=load(parameters+0x2cu),azimuth=load(parameters+0x28u);
      kernels=services.select_kernel(azimuth,elevation,fold);
    }
    const auto old_bank=std::to_integer<std::uint32_t>(memory(voice+0x66u,1)[0]);
    const auto raw_handle=memory(voice+0xcu,2);
    const auto handle=std::to_integer<std::uint32_t>(raw_handle[0])|
        (std::to_integer<std::uint32_t>(raw_handle[1])<<8);
    const auto entry=(handle*2u+old_bank)&0xffffu;
    const auto new_bank=std::byte(old_bank==0);
    memory(voice+0x66u,1)[0]=new_bank;
    if(observer)observer({0x38138cu,voice+0x66u,1,old_bank,std::to_integer<std::uint32_t>(new_bank)});
    if(!services.fifo_available)throw std::runtime_error("Original HRTF device FIFO provider unavailable");
    while((services.fifo_available()&0xfffffffcu)<0x4cu){}
    if(!services.device_write)throw std::runtime_error("Original HRTF device command provider unavailable");
    services.device_write(0xfe820160u,entry);
    if(!services.kernel_bytes)throw std::runtime_error("Original HRTF selected kernel bytes unavailable");
    const auto left=services.kernel_bytes(kernels[0]),right=services.kernel_bytes(kernels[1]);
    const auto byte=[](std::byte value){return std::to_integer<std::uint32_t>(value);};
    for(unsigned i=0;i<15;++i){
      const auto value=byte(left[i*2])|(byte(right[i*2])<<8)|
          (byte(left[i*2+1])<<16)|(byte(right[i*2+1])<<24);
      services.device_write(0xfe820400u+i*4u,value);
    }
    std::uint32_t parameter=0;
    if(parameters){
      const auto status=compare(real(word(parameters+0x28u)),real(constants.zero_bits));
      parameter=status&0x100u?0u-byte(right[31]):byte(left[31]);
    }
    services.device_write(0xfe82043cu,(parameter<<25)|byte(left[30])|(byte(right[30])<<8));
    // The original reloads this retained handle after coefficient publication.
    const auto selected=memory(voice+0xcu,2);
    services.device_write(0xfe8202f8u,byte(selected[0])|(byte(selected[1])<<8));
    services.device_write(0xfe82031cu,entry);
  });
  if(invocations!=1)throw std::runtime_error("Original HRTF device lock callback not executed");
#else
  (void)voice;(void)source_override;(void)parameters_override;(void)bindings;
  (void)constants;(void)fp;(void)services;(void)observer;unavailable();
#endif
}
} // namespace halo2
