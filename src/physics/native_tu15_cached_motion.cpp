#include "halo2/native_tu15_cached_motion.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using Vector=NativeTu15PhysicsVector;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Memory {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
  Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
  std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
  Vector vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 cached motion requires aligned vector storage");std::array<std::byte,16>b{};m_.read_vector(p,b);Vector v{};std::memcpy(v.data(),b.data(),16);return v;}
  void publish(std::uint32_t pc,std::uint32_t p,std::span<const std::byte>b){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(b.size());m_.read(p,event.before);event.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(event);}
  void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));publish(pc,p,b);}
  void vector(std::uint32_t pc,std::uint32_t p,const Vector&v){if(p&15u)throw std::runtime_error("Native TU1.5 cached motion requires aligned vector storage");std::array<std::byte,16>b{};std::memcpy(b.data(),v.data(),16);publish(pc,p,b);}
  void scalar(std::uint32_t pc,std::uint32_t p){SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(4);m_.read(p,event.before);}alignas(16)std::array<std::byte,108>prior{};std::uint32_t n;asm volatile("fnsave %0\n\tfrstor %0":"=m"(prior)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");asm volatile("fstps %0":"=m"(n)::"memory","st");std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));try{m_.write(p,b);}catch(...){asm volatile("frstor %0"::"m"(prior):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;}if(o_){event.written.assign(b.begin(),b.end());o_(event);}}
};
class Storage {
  Memory&m_;std::uint32_t motion_;
public:
  bool advancing{};
  Storage(Memory&m,std::uint32_t motion):m_(m),motion_(motion){}
  Vector position()const{return m_.vector(motion_+0x70u);}Vector linear_velocity()const{return m_.vector(motion_+0xc0u);}Vector angular_velocity()const{return m_.vector(motion_+0xd0u);}Vector quaternion()const{return m_.vector(motion_+0x30u);}Vector local_center()const{return m_.vector(motion_+0x20u);}Vector xyz_mask()const{return m_.vector(0x41f1d0u);}
  std::uint32_t quaternion_component(unsigned n)const{return m_.word(motion_+0x30u+n*4u);}std::uint32_t radius()const{return m_.word(motion_+0xcu);}std::uint32_t cached_fraction()const{return m_.word(motion_+0x7cu);}std::uint32_t one()const{return m_.word(0x46a0d0u);}std::uint32_t normalization_three()const{return m_.word(0x46a0e4u);}std::uint32_t normalization_half()const{return m_.word(0x46a0ccu);}
  void previous_position(const Vector&v){m_.vector(advancing?0x32595au:0x32582eu,motion_+0x60u,v);}void position(const Vector&v){m_.vector(advancing?0x32597au:0x325858u,motion_+0x70u,v);}void angular_travel(){m_.scalar(advancing?0x3259e1u:0x3258c0u,motion_+0x6cu);}
  void quaternion(const Vector&v,bool normalized){m_.vector(normalized?0x1e1075u:advancing?0x325a1bu:0x325907u,motion_+0x30u,v);}
  static std::uint32_t rotation_site(unsigned n){constexpr std::array<std::uint32_t,12>sites{0x37c644,0x37c64b,0x37c654,0x37c657,0x37c65f,0x37c672,0x37c67d,0x37c680,0x37c687,0x37c692,0x37c6a3,0x37c6a6};return sites.at(n);}
  void rotation_scalar(unsigned n){m_.scalar(rotation_site(n),motion_+0x80u+n*4u);}void rotation_zero(unsigned n){m_.word(rotation_site(n),motion_+0x80u+n*4u,0);}
  void world_origin(const Vector&v){m_.vector(advancing?0x325a49u:0x32593du,motion_+0xb0u,v);}void fraction_one(){m_.word(0x325944u,motion_+0x7cu,0x3f800000u);}void fraction(std::uint32_t n){m_.word(0x325a50u,motion_+0x7cu,n);}
};
class Environment {
  alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
    if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 cached motion requires its unsupported x87/SSE fault owner");
    mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::memcpy(native_.data(),host_.data(),28);const std::uint16_t empty=0xffff;
    std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr_);}
};
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
Vector words(__m128 v){Vector out;_mm_storeu_si128(reinterpret_cast<__m128i*>(out.data()),_mm_castps_si128(v));return out;}
__m128 broadcast(std::uint32_t n){return _mm_castsi128_ps(_mm_set1_epi32(std::bit_cast<std::int32_t>(n)));}
// Even strict C++ floating point permits commutative intrinsic operands to be
// swapped. These native instructions lock destination/source priority for
// distinct NaN payloads as well as the original operation grouping.
__m128 add_ps(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub_ps(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul_ps(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 add_ss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 sub_ss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mul_ss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
void push(std::uint32_t n){asm volatile("flds %0"::"m"(n):"memory","st");}
void multiply(std::uint32_t n){asm volatile("fmuls %0"::"m"(n):"memory","st");}
std::uint32_t spill(){std::uint32_t out;asm volatile("fstps %0":"=m"(out)::"memory","st");return out;}
std::uint32_t subtract_and_spill(std::uint32_t n){std::uint32_t out;asm volatile("fsubs %1\n\tfstps %0":"=m"(out):"m"(n):"memory","st");return out;}
// Actual323430 product with private native vectors. Keep both persistent
// quaternion re-reads and the three cross/scale additions in source order.
Vector quaternion_product(Storage&s,const Vector&a){
  auto b=simd(s.quaternion());const auto av=simd(a);
  const auto cross_right=mul_ps(_mm_shuffle_ps(av,av,0xd2),_mm_shuffle_ps(b,b,0xc9));
  auto result=sub_ps(mul_ps(_mm_shuffle_ps(av,av,0xc9),_mm_shuffle_ps(b,b,0xd2)),cross_right);
  b=simd(s.quaternion());result=add_ps(result,mul_ps(broadcast(a[3]),b));
  const auto bw=s.quaternion_component(3);result=add_ps(result,mul_ps(broadcast(bw),av));
  push(a[3]);b=simd(s.quaternion());multiply(s.quaternion_component(3));
  const auto dot=mul_ps(av,b);const auto xy=add_ss(_mm_shuffle_ps(dot,dot,0x55),dot);
  const auto xyz=add_ss(_mm_shuffle_ps(dot,dot,0xaa),xy);auto out=words(result);
  out[3]=subtract_and_spill(words(xyz)[0]);return out;
}
void normalize(Storage&s,const NativeTu15PoseRsqrtObservation&observe){
  const auto q=simd(s.quaternion());const auto three=s.normalization_three();const auto square=mul_ps(q,q);
  auto sum=add_ps(_mm_shuffle_ps(square,square,0x4e),square);
  sum=add_ps(sum,_mm_shuffle_ps(sum,sum,0xb1));
  // Real native RSQRTSS, then the exact original scalar Newton sequence.
  // No std::sqrt/divide replacement and no injected Xbox estimate result.
  const auto estimate=_mm_rsqrt_ss(sum);if(observe)observe(words(sum)[0],words(estimate)[0]);
  auto correction=mul_ss(sum,estimate);correction=mul_ss(correction,estimate);
  correction=sub_ss(broadcast(three),correction);
  auto scale=mul_ss(broadcast(s.normalization_half()),estimate);scale=mul_ss(scale,correction);
  s.quaternion(words(mul_ps(_mm_shuffle_ps(scale,scale,0),q)),true);
}
// Named native x87 expressions retain the two doubled components and their W
// products as real host extended intermediates, including the original binary32
// Z spill. No virtual x87 register array or guest instruction execution exists.
void doubled_component(std::uint32_t n){asm volatile("flds %0\n\tfadd %%st(0),%%st"::"m"(n):"memory","st");}
std::uint32_t doubled_z(std::uint32_t n){std::uint32_t out;asm volatile("flds %1\n\tfadd %%st(0),%%st\n\tfstps %0":"=m"(out):"m"(n):"memory","st");return out;}
std::uint32_t x_product(Storage&s,unsigned lane){asm volatile("fld %%st(1)":::"memory","st");multiply(s.quaternion_component(lane));return spill();}
std::uint32_t y_product(Storage&s,unsigned lane){asm volatile("fld %%st(0)":::"memory","st");multiply(s.quaternion_component(lane));return spill();}
std::uint32_t z_product(Storage&s,std::uint32_t z2,unsigned lane){push(z2);multiply(s.quaternion_component(lane));return spill();}
std::uint32_t x_w(Storage&s){asm volatile("fxch %%st(1)":::"memory","st");multiply(s.quaternion_component(3));return spill();}
void discard_zw(){asm volatile("fstp %%st(0)":::"memory","st");}
void push_diagonal(Storage&s,std::uint32_t a,std::uint32_t b){asm volatile("flds %0\n\tfadds %1"::"m"(a),"m"(b):"memory","st");const auto one=s.one();asm volatile("fsubrs %0"::"m"(one):"memory","st");}
void push_xy_plus_zw(std::uint32_t xy){asm volatile("flds %0\n\tfadd %%st(1),%%st"::"m"(xy):"memory","st");}
void push_xz_minus_yw(std::uint32_t xz){asm volatile("flds %0\n\tfsub %%st(2),%%st"::"m"(xz):"memory","st");}
void push_xy_minus_zw(std::uint32_t xy){asm volatile("flds %0\n\tfsub %%st(1),%%st"::"m"(xy):"memory","st");}
void push_sum(std::uint32_t a,std::uint32_t b){asm volatile("flds %0\n\tfadds %1"::"m"(a),"m"(b):"memory","st");}
void push_difference(std::uint32_t a,std::uint32_t b){asm volatile("flds %0\n\tfsubs %1"::"m"(a),"m"(b):"memory","st");}
void quaternion_matrix(Storage&s){
  doubled_component(s.quaternion_component(0));doubled_component(s.quaternion_component(1));const auto z2=doubled_z(s.quaternion_component(2));
  const auto xx=x_product(s,0),xy=y_product(s,0),xz=z_product(s,z2,0);const auto yy=y_product(s,1),yz=z_product(s,z2,1),zz=z_product(s,z2,2);
  const auto xw=x_w(s);multiply(s.quaternion_component(3));push(z2);multiply(s.quaternion_component(3));
  push_diagonal(s,zz,yy);s.rotation_scalar(0);push_xy_plus_zw(xy);s.rotation_scalar(1);push_xz_minus_yw(xz);s.rotation_scalar(2);s.rotation_zero(3);
  push_xy_minus_zw(xy);s.rotation_scalar(4);discard_zw();push_diagonal(s,zz,xx);s.rotation_scalar(5);push_sum(xw,yz);s.rotation_scalar(6);s.rotation_zero(7);
  asm volatile("fadds %0"::"m"(xz):"memory","st");s.rotation_scalar(8);push_difference(yz,xw);s.rotation_scalar(9);push_diagonal(s,yy,xx);s.rotation_scalar(10);s.rotation_zero(11);
}

// Actual2E5810 rotates the local center through the quaternion, retaining the
// source x87 W and dot intermediates. Its private output is never a body field.
Vector rotate_local_center(Storage&s){
  const auto center=simd(s.local_center());push(s.quaternion_component(3));
  std::uint32_t scale;asm volatile("fld %%st(0); fmul %%st(1); fadd %%st(0),%%st":::"memory","st");
  const auto one=s.one();asm volatile("fsubs %1; fstps %0":"=m"(scale):"m"(one):"memory","st");
  auto result=mul_ps(broadcast(scale),center);auto quaternion=s.quaternion();quaternion[3]=0;const auto q=simd(quaternion);
  const auto products=mul_ps(q,center);const auto xy=add_ss(_mm_shuffle_ps(products,products,0x55),products);
  const auto dot=words(add_ss(_mm_shuffle_ps(products,products,0xaa),xy))[0];
  push(dot);std::uint32_t twice_dot,twice_w;
  asm volatile("fadd %%st(0),%%st; fstps %0; fadd %%st(0),%%st; fstps %1":"=m"(twice_dot),"=m"(twice_w)::"memory","st");
  const auto right=mul_ps(_mm_shuffle_ps(q,q,0xd2),_mm_shuffle_ps(center,center,0xc9));
  const auto cross=sub_ps(mul_ps(_mm_shuffle_ps(q,q,0xc9),_mm_shuffle_ps(center,center,0xd2)),right);
  result=add_ps(result,mul_ps(broadcast(twice_dot),q));
  return words(add_ps(result,mul_ps(broadcast(twice_w),cross)));
}

void negate(){asm volatile("fchs":::"memory","st");}
void multiply_second(){asm volatile("fmul %%st(1),%%st(0)":::"memory","st","st(1)");}
void discard(){asm volatile("fstp %%st(0)":::"memory","st");}
std::uint32_t keep(){std::uint32_t value;asm volatile("fsts %0":"=m"(value)::"memory","st");return value;}
__m128 length3(Storage&s,__m128&angular_for_half){const auto angular=simd(s.angular_velocity());const auto square=mul_ps(angular,angular),xy=add_ss(_mm_shuffle_ps(square,square,0x55),square);const auto z=_mm_shuffle_ps(square,square,0xaa);angular_for_half=simd(s.angular_velocity());auto value=add_ss(z,xy);asm volatile("sqrtss %0,%0":"+x"(value)::"memory");return value;}
#endif
}
void native_tu15_advance_cached_motion(SourcePhysicsContactMemory&memory,NativeTu15PhysicsMotion motion,std::uint32_t fraction,std::uint32_t seconds,NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,const NativeTu15PoseRsqrtObservation&rsqrt,NativeTu15CachedMotionStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp,depth);Memory m(memory,observe);Storage s(m,motion.identity);if(stats)++stats->calls;
  // Rewind the current cached fraction. Keep its extended negative product
  // alive for angular travel, while linear/quaternion math uses its f32 spill.
  push(seconds);multiply(s.cached_fraction());const auto previous=s.position();const auto linear=simd(s.linear_velocity());s.previous_position(previous);negate();const auto rewind=keep();
  const auto position=simd(s.position());s.position(words(add_ps(position,mul_ps(broadcast(rewind),linear))));__m128 angular;const auto angular_length=length3(s,angular);push(words(angular_length)[0]);multiply(s.radius());multiply_second();s.angular_travel();const auto mask=simd(s.xyz_mask());discard();
  const auto half_angular=words(_mm_and_ps(mul_ps(broadcast(0x3f000000u),angular),mask));const auto first_product=simd(quaternion_product(s,half_angular)),first_quaternion=simd(s.quaternion());s.quaternion(words(add_ps(first_quaternion,mul_ps(broadcast(rewind),first_product))),false);normalize(s,rsqrt);quaternion_matrix(s);const auto first_center=simd(rotate_local_center(s));
  // Source begins the new extended product before publishing the first origin
  // and temporary cached1, so rejected stores retain that genuine private value.
  push(fraction);const auto first_position=simd(s.position());multiply(seconds);s.world_origin(words(sub_ps(first_position,first_center)));if(stats)++stats->rewinds;s.fraction_one();s.advancing=true;
  const auto next_previous=s.position();const auto advance=keep();const auto next_linear=simd(s.linear_velocity());s.previous_position(next_previous);const auto next_position=simd(s.position());s.position(words(add_ps(next_position,mul_ps(broadcast(advance),next_linear))));
  __m128 next_angular;const auto next_angular_length=length3(s,next_angular);push(words(next_angular_length)[0]);multiply(s.radius());multiply_second();const auto scaled_angular=mul_ps(broadcast(0x3f000000u),next_angular);s.angular_travel();const auto next_mask=simd(s.xyz_mask());const auto next_half_angular=words(_mm_and_ps(scaled_angular,next_mask));discard();
  const auto second_product=simd(quaternion_product(s,next_half_angular)),second_quaternion=simd(s.quaternion());s.quaternion(words(add_ps(second_quaternion,mul_ps(broadcast(advance),second_product))),false);normalize(s,rsqrt);quaternion_matrix(s);const auto second_center=simd(rotate_local_center(s)),last_position=simd(s.position());s.world_origin(words(sub_ps(last_position,second_center)));s.fraction(fraction);if(stats)++stats->advances;
#else
  (void)memory;(void)motion;(void)fraction;(void)seconds;(void)fp;(void)depth;(void)observe;(void)rsqrt;(void)stats;throw std::runtime_error("Exact native TU1.5 cached motion backend unavailable");
#endif
}
} // namespace halo2
