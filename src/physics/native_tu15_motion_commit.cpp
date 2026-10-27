#include "halo2/native_tu15_motion_commit.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using Storage=NativeTu15MotionCommitStorage;using Vector=NativeTu15PhysicsVector;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
    if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 motion commit requires its unsupported x87/SSE fault owner");
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
std::uint32_t diagonal(Storage&s,std::uint32_t a,std::uint32_t b){asm volatile("flds %0\n\tfadds %1"::"m"(a),"m"(b):"memory","st");const auto one=s.one();std::uint32_t out;asm volatile("fsubrs %1\n\tfstps %0":"=m"(out):"m"(one):"memory","st");return out;}
std::uint32_t xy_plus_zw(std::uint32_t xy){std::uint32_t out;asm volatile("flds %1\n\tfadd %%st(1),%%st\n\tfstps %0":"=m"(out):"m"(xy):"memory","st");return out;}
std::uint32_t xz_minus_yw(std::uint32_t xz){std::uint32_t out;asm volatile("flds %1\n\tfsub %%st(2),%%st\n\tfstps %0":"=m"(out):"m"(xz):"memory","st");return out;}
std::uint32_t xy_minus_zw(std::uint32_t xy){std::uint32_t out;asm volatile("flds %1\n\tfsub %%st(1),%%st\n\tfstps %0":"=m"(out):"m"(xy):"memory","st");return out;}
void discard_zw(){asm volatile("fstp %%st(0)":::"memory","st");}
std::uint32_t scalar_sum(std::uint32_t a,std::uint32_t b){std::uint32_t out;asm volatile("flds %1\n\tfadds %2\n\tfstps %0":"=m"(out):"m"(a),"m"(b):"memory","st");return out;}
std::uint32_t yw_plus_xz(std::uint32_t xz){std::uint32_t out;asm volatile("fadds %1\n\tfstps %0":"=m"(out):"m"(xz):"memory","st");return out;}
std::uint32_t scalar_difference(std::uint32_t a,std::uint32_t b){std::uint32_t out;asm volatile("flds %1\n\tfsubs %2\n\tfstps %0":"=m"(out):"m"(a),"m"(b):"memory","st");return out;}
void quaternion_matrix(Storage&s){
  doubled_component(s.quaternion_component(0));doubled_component(s.quaternion_component(1));const auto z2=doubled_z(s.quaternion_component(2));
  const auto xx=x_product(s,0);const auto xy=y_product(s,0);const auto xz=z_product(s,z2,0);
  const auto yy=y_product(s,1);const auto yz=z_product(s,z2,1);const auto zz=z_product(s,z2,2);
  const auto xw=x_w(s);multiply(s.quaternion_component(3));push(z2);multiply(s.quaternion_component(3));
  s.rotation_component(0,diagonal(s,zz,yy));s.rotation_component(1,xy_plus_zw(xy));s.rotation_component(2,xz_minus_yw(xz));s.rotation_component(3,0);
  s.rotation_component(4,xy_minus_zw(xy));discard_zw();s.rotation_component(5,diagonal(s,zz,xx));s.rotation_component(6,scalar_sum(xw,yz));s.rotation_component(7,0);
  s.rotation_component(8,yw_plus_xz(xz));s.rotation_component(9,scalar_difference(yz,xw));s.rotation_component(10,diagonal(s,yy,xx));s.rotation_component(11,0);
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
#endif
}
NativeTu15ConstraintAccumulator native_tu15_commit_motion(NativeTu15MotionCommitKind kind,Storage&s,std::uint32_t parameter_bits,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15PoseRsqrtObservation&observe){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(kind!=NativeTu15MotionCommitKind::kinematic&&kind!=NativeTu15MotionCommitKind::dynamic&&kind!=NativeTu15MotionCommitKind::restore_saved_velocity)throw std::runtime_error("Unknown native TU1.5 motion commit owner");
  Environment environment(fp,depth);
  const auto next=NativeTu15ConstraintAccumulator{s.accumulator().identity+0x80u};
  // Source rotation order is Z,Y,(their products),X; each multiply keeps the
  // matrix operand as destination, then additions group (X+Y)+Z.
  auto world_velocity=[&](const Vector&local){
    const auto v=simd(local),z=simd(s.rotation(2)),y=simd(s.rotation(1));
    const auto scaled_z=mul_ps(z,_mm_shuffle_ps(v,v,0xaa));
    const auto scaled_y=mul_ps(y,_mm_shuffle_ps(v,v,0x55));
    const auto x=simd(s.rotation(0));
    return add_ps(add_ps(mul_ps(x,_mm_shuffle_ps(v,v,0)),scaled_y),scaled_z);
  };
  __m128 displacement_velocity;
  if(kind==NativeTu15MotionCommitKind::kinematic){
    // Actual3235F0 does not touch either accumulator payload or parameter.
    const auto previous=s.position();displacement_velocity=simd(s.linear_velocity());s.previous_position(previous);
    const auto dt=s.seconds();const auto position=simd(s.position());
    const auto displacement=mul_ps(broadcast(dt),displacement_velocity);s.position(words(add_ps(position,displacement)));
  }else{
    const auto linear=simd(s.accumulator_working_linear());
    s.working_linear_velocity(words(mul_ps(broadcast(parameter_bits),linear)));
    const auto angular=world_velocity(s.accumulator_working_angular());
    const auto working_angular=mul_ps(broadcast(parameter_bits),angular);
    if(kind==NativeTu15MotionCommitKind::restore_saved_velocity){
      // Actual3255D0 retains this linear read BEFORE its angular store. It
      // must survive that store and the subsequent previous-velocity writes.
      displacement_velocity=simd(s.linear_velocity());
      s.working_angular_velocity(words(working_angular));
      const auto current_angular=s.angular_velocity();
      s.previous_linear_velocity(words(displacement_velocity));s.previous_angular_velocity(current_angular);
    }else{
      s.working_angular_velocity(words(working_angular));
      const auto linear_mask=simd(s.xyz_mask());const auto current_linear=simd(s.linear_velocity());
      s.masked_linear_velocity(words(_mm_and_ps(current_linear,linear_mask)));
      const auto angular_mask=simd(s.xyz_mask());const auto current_angular=simd(s.angular_velocity());
      s.masked_angular_velocity(words(_mm_and_ps(current_angular,angular_mask)));
      const auto previous_angular=s.angular_velocity();displacement_velocity=simd(s.linear_velocity());
      s.previous_linear_velocity(words(displacement_velocity));s.previous_angular_velocity(previous_angular);
    }
    s.previous_position(s.position());const auto dt=s.seconds();
    const auto displacement=mul_ps(broadcast(dt),displacement_velocity);const auto position=simd(s.position());
    s.position(words(add_ps(position,displacement)));
  }
  push(s.radius());const auto angular=simd(s.angular_velocity());multiply(s.seconds());
  const auto square=mul_ps(angular,angular);const auto xy=add_ss(_mm_shuffle_ps(square,square,0x55),square);
  const auto length=_mm_sqrt_ss(add_ss(_mm_shuffle_ps(square,square,0xaa),xy));multiply(words(length)[0]);s.angular_travel(spill());
  __m128 half_angular,mask;
  if(kind==NativeTu15MotionCommitKind::dynamic){mask=simd(s.xyz_mask());half_angular=simd(s.angular_velocity());}
  else{half_angular=simd(s.angular_velocity());mask=simd(s.xyz_mask());}
  const auto half=words(_mm_and_ps(mul_ps(broadcast(0x3f000000u),half_angular),mask));
  const auto product=simd(quaternion_product(s,half));const auto quaternion_dt=s.seconds();const auto quaternion=simd(s.quaternion());
  s.quaternion(words(add_ps(quaternion,mul_ps(broadcast(quaternion_dt),product))),false);
  normalize(s,observe);quaternion_matrix(s);
  if(kind==NativeTu15MotionCommitKind::restore_saved_velocity){
    // Source3255D0 reads position after the three products but BEFORE the
    // two additions. Keep that fault/FP boundary distinct from the angular
    // velocity transform above, which has no intervening position read.
    const auto center=simd(s.local_center()),z=simd(s.rotation(2)),y=simd(s.rotation(1));
    const auto scaled_z=mul_ps(z,_mm_shuffle_ps(center,center,0xaa));
    const auto scaled_y=mul_ps(y,_mm_shuffle_ps(center,center,0x55));
    const auto scaled_x=mul_ps(simd(s.rotation(0)),_mm_shuffle_ps(center,center,0));
    const auto position=simd(s.position());
    s.world_origin(words(sub_ps(position,add_ps(add_ps(scaled_x,scaled_y),scaled_z))));s.position_w_one();
    s.restored_linear_velocity(s.accumulator_saved_linear());
    s.restored_angular_velocity(words(world_velocity(s.accumulator_saved_angular())));
    const auto linear_mask=simd(s.xyz_mask());const auto restored_linear=simd(s.linear_velocity());
    s.masked_linear_velocity(words(_mm_and_ps(restored_linear,linear_mask)));
    // The source reads angular velocity BEFORE this second mask operand.
    const auto restored_angular=simd(s.angular_velocity());const auto angular_mask=simd(s.xyz_mask());
    s.masked_angular_velocity(words(_mm_and_ps(restored_angular,angular_mask)));
  }else{
    const auto rotated=simd(rotate_local_center(s));const auto position=simd(s.position());
    s.world_origin(words(sub_ps(position,rotated)));s.position_w_one();
  }
  return next;
#else
  (void)kind;(void)s;(void)parameter_bits;(void)fp;(void)depth;(void)observe;throw std::runtime_error("Exact native TU1.5 motion commit backend unavailable");
#endif
}
NativeTu15MotionCommitMemory::NativeTu15MotionCommitMemory(SourcePhysicsContactMemory&m,NativeTu15MotionCommitKind kind,NativeTu15PhysicsMotion motion,NativeTu15PhysicsStepPacket step,NativeTu15ConstraintAccumulator accumulator,const SourcePhysicsContactRecordObserver&o):memory_(m),kind_(kind),motion_(motion.identity),step_(step.identity),accumulator_(accumulator.identity),observer_(o){}
std::uint32_t NativeTu15MotionCommitMemory::scalar(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
Vector NativeTu15MotionCommitMemory::vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector out{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)out[k]|=std::to_integer<unsigned>(b[k*4u+i])<<(i*8u);return out;}
void NativeTu15MotionCommitMemory::scalar(std::uint32_t instruction,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(n>>(i*8u));SourcePhysicsContactRecordStore store{instruction,p,{},{}};if(observer_){store.before.resize(4);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
void NativeTu15MotionCommitMemory::vector(std::uint32_t instruction,std::uint32_t p,const Vector&v){std::array<std::byte,16>b{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)b[k*4u+i]=std::byte(v[k]>>(i*8u));SourcePhysicsContactRecordStore store{instruction,p,{},{}};if(observer_){store.before.resize(16);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
std::uint32_t NativeTu15MotionCommitMemory::site(std::uint32_t kinematic,std::uint32_t dynamic,std::uint32_t restored)const{const auto instruction=kind_==NativeTu15MotionCommitKind::kinematic?kinematic:kind_==NativeTu15MotionCommitKind::dynamic?dynamic:kind_==NativeTu15MotionCommitKind::restore_saved_velocity?restored:0u;if(!instruction)throw std::runtime_error("Unavailable native TU1.5 motion commit store");return instruction;}
NativeTu15ConstraintAccumulator NativeTu15MotionCommitMemory::accumulator()const{return{accumulator_};}
Vector NativeTu15MotionCommitMemory::accumulator_working_linear()const{return vector(accumulator_+0x40u);}
Vector NativeTu15MotionCommitMemory::accumulator_working_angular()const{return vector(accumulator_+0x50u);}
Vector NativeTu15MotionCommitMemory::accumulator_saved_linear()const{return vector(accumulator_+0x10u);}
Vector NativeTu15MotionCommitMemory::accumulator_saved_angular()const{return vector(accumulator_+0x20u);}
Vector NativeTu15MotionCommitMemory::angular_velocity()const{return vector(motion_+0x50u);}
Vector NativeTu15MotionCommitMemory::linear_velocity()const{return vector(motion_+0x40u);}
Vector NativeTu15MotionCommitMemory::position()const{return vector(motion_+0x70u);}
Vector NativeTu15MotionCommitMemory::quaternion()const{return vector(motion_+0x30u);}
std::uint32_t NativeTu15MotionCommitMemory::quaternion_component(unsigned n)const{if(n>3)throw std::runtime_error("Unknown native commit quaternion lane");return scalar(motion_+0x30u+n*4u);}
Vector NativeTu15MotionCommitMemory::local_center()const{return vector(motion_+0x20u);}
Vector NativeTu15MotionCommitMemory::rotation(unsigned n)const{if(n>2)throw std::runtime_error("Unknown native commit rotation axis");return vector(motion_+0x80u+n*16u);}
Vector NativeTu15MotionCommitMemory::xyz_mask()const{return vector(0x41f1d0u);}
std::uint32_t NativeTu15MotionCommitMemory::seconds()const{return scalar(step_);}
std::uint32_t NativeTu15MotionCommitMemory::radius()const{return scalar(motion_+0xcu);}
std::uint32_t NativeTu15MotionCommitMemory::one()const{return scalar(0x46a0d0u);}
std::uint32_t NativeTu15MotionCommitMemory::normalization_three()const{return scalar(0x46a0e4u);}
std::uint32_t NativeTu15MotionCommitMemory::normalization_half()const{return scalar(0x46a0ccu);}
void NativeTu15MotionCommitMemory::working_linear_velocity(Vector v){vector(site(0,0x324a09u,0x3255f9u),motion_+0x40u,v);}
void NativeTu15MotionCommitMemory::working_angular_velocity(Vector v){vector(site(0,0x324a5du,0x325652u),motion_+0x50u,v);}
void NativeTu15MotionCommitMemory::masked_linear_velocity(Vector v){vector(site(0,0x324a6fu,0x3257e4u),motion_+0x40u,v);}
void NativeTu15MotionCommitMemory::masked_angular_velocity(Vector v){vector(site(0,0x324a81u,0x3257f7u),motion_+0x50u,v);}
void NativeTu15MotionCommitMemory::restored_linear_velocity(Vector v){vector(site(0,0,0x32579bu),motion_+0x40u,v);}
void NativeTu15MotionCommitMemory::restored_angular_velocity(Vector v){vector(site(0,0,0x3257d2u),motion_+0x50u,v);}
void NativeTu15MotionCommitMemory::previous_linear_velocity(Vector v){vector(site(0,0x324a8du,0x32565au),motion_+0xc0u,v);}
void NativeTu15MotionCommitMemory::previous_angular_velocity(Vector v){vector(site(0,0x324a94u,0x325661u),motion_+0xd0u,v);}
void NativeTu15MotionCommitMemory::previous_position(Vector v){vector(site(0x323608u,0x324a9fu,0x32566cu),motion_+0x60u,v);}
void NativeTu15MotionCommitMemory::position(Vector v){vector(site(0x323629u,0x324ac0u,0x32568du),motion_+0x70u,v);}
void NativeTu15MotionCommitMemory::angular_travel(std::uint32_t n){scalar(site(0x323681u,0x324b17u,0x3256e4u),motion_+0x6cu,n);}
void NativeTu15MotionCommitMemory::quaternion(Vector v,bool normalized){vector(normalized?0x1e1075u:site(0x3236d6u,0x324b70u,0x32573cu),motion_+0x30u,v);}
void NativeTu15MotionCommitMemory::rotation_component(unsigned n,std::uint32_t v){constexpr std::array<std::uint32_t,12>instructions{0x37c644,0x37c64b,0x37c654,0x37c657,0x37c65f,0x37c672,0x37c67d,0x37c680,0x37c687,0x37c692,0x37c6a3,0x37c6a6};scalar(instructions.at(n),motion_+0x80u+n*4u,v);}
void NativeTu15MotionCommitMemory::world_origin(Vector v){vector(site(0x323707u,0x324b9du,0x325789u),motion_+0xb0u,v);}
void NativeTu15MotionCommitMemory::position_w_one(){scalar(site(0x32370fu,0x324ba5u,0x325790u),motion_+0x7cu,0x3f800000u);}
} // namespace halo2
