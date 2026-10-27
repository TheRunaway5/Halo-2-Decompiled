#include "halo2/native_tu15_kinematic_motion.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using Storage=NativeTu15KinematicPoseStorage;using Vector=NativeTu15PhysicsVector;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};std::array<std::byte,28>native_{};unsigned mxcsr_{};NativeTu15PhysicsFrameFp&fp_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
    if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 pose requires its unsupported x87/SSE fault owner");
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
void native_tu15_integrate_kinematic_motion_pose(Storage&s,NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15PoseRsqrtObservation&observe,NativeTu15MotionVariantActivity*activity){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp,depth);
  const auto previous=s.position();const auto linear=simd(s.linear_velocity());s.previous_position(previous);
  const auto dt=s.seconds();const auto position=simd(s.position());const auto displacement=mul_ps(broadcast(dt),linear);
  s.position(words(add_ps(position,displacement)));
  push(s.radius());const auto angular=simd(s.angular_velocity());multiply(s.seconds());const auto square=mul_ps(angular,angular);
  const auto xy=add_ss(_mm_shuffle_ps(square,square,0x55),square);const auto length=_mm_sqrt_ss(add_ss(_mm_shuffle_ps(square,square,0xaa),xy));multiply(words(length)[0]);s.angular_travel(spill());
  const auto half_angular=simd(s.angular_velocity()),mask=simd(s.xyz_mask());
  const auto half=words(_mm_and_ps(mul_ps(broadcast(0x3f000000u),half_angular),mask));
  const auto product=simd(quaternion_product(s,half));const auto quaternion_dt=s.seconds();const auto quaternion=simd(s.quaternion());
  s.quaternion(words(add_ps(quaternion,mul_ps(broadcast(quaternion_dt),product))),false);
  normalize(s,observe);quaternion_matrix(s);
  const auto rotated=simd(rotate_local_center(s));s.world_origin(words(sub_ps(simd(s.position()),rotated)));s.position_w_one();
  if(activity)++activity->kinematic_pose_calls;
#else
  (void)s;(void)fp;(void)depth;(void)observe;(void)activity;throw std::runtime_error("Exact native TU1.5 kinematic pose backend unavailable");
#endif
}
void native_tu15_apply_prescribed_motion_forces(NativeTu15PhysicsFrameFp&fp,NativeTu15MotionVariantActivity*activity){(void)fp;if(activity)++activity->prescribed_force_calls;}
void native_tu15_keep_fixed_motion_pose(NativeTu15PhysicsFrameFp&fp,NativeTu15MotionVariantActivity*activity){(void)fp;if(activity)++activity->fixed_pose_calls;}
NativeTu15KinematicPoseMemory::NativeTu15KinematicPoseMemory(SourcePhysicsContactMemory&m,std::uint32_t motion,std::uint32_t step,const SourcePhysicsContactRecordObserver&o):memory_(m),motion_(motion),step_(step),observer_(o){}
std::uint32_t NativeTu15KinematicPoseMemory::scalar(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
Vector NativeTu15KinematicPoseMemory::vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector out{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)out[k]|=std::to_integer<unsigned>(b[k*4u+i])<<(i*8u);return out;}
void NativeTu15KinematicPoseMemory::scalar(std::uint32_t site,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(n>>(i*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(4);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
void NativeTu15KinematicPoseMemory::vector(std::uint32_t site,std::uint32_t p,const Vector&v){std::array<std::byte,16>b{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)b[k*4u+i]=std::byte(v[k]>>(i*8u));SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(16);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}memory_.write(p,b);if(observer_)observer_(store);}
Vector NativeTu15KinematicPoseMemory::angular_velocity()const{return vector(motion_+0x50);}
Vector NativeTu15KinematicPoseMemory::linear_velocity()const{return vector(motion_+0x40);}
Vector NativeTu15KinematicPoseMemory::position()const{return vector(motion_+0x70);}
Vector NativeTu15KinematicPoseMemory::quaternion()const{return vector(motion_+0x30);}
std::uint32_t NativeTu15KinematicPoseMemory::quaternion_component(unsigned n)const{if(n>3)throw std::runtime_error("Unknown native pose quaternion lane");return scalar(motion_+0x30+n*4u);}
Vector NativeTu15KinematicPoseMemory::local_center()const{return vector(motion_+0x20);}
Vector NativeTu15KinematicPoseMemory::xyz_mask()const{return vector(0x41f1d0);}
std::uint32_t NativeTu15KinematicPoseMemory::seconds()const{return scalar(step_);}
std::uint32_t NativeTu15KinematicPoseMemory::radius()const{return scalar(motion_+0xc);}
std::uint32_t NativeTu15KinematicPoseMemory::one()const{return scalar(0x46a0d0);}
std::uint32_t NativeTu15KinematicPoseMemory::normalization_three()const{return scalar(0x46a0e4);}
std::uint32_t NativeTu15KinematicPoseMemory::normalization_half()const{return scalar(0x46a0cc);}
void NativeTu15KinematicPoseMemory::previous_position(Vector v){vector(0x323748,motion_+0x60,v);}
void NativeTu15KinematicPoseMemory::position(Vector v){vector(0x323769,motion_+0x70,v);}
void NativeTu15KinematicPoseMemory::angular_travel(std::uint32_t n){scalar(0x3237c1,motion_+0x6c,n);}
void NativeTu15KinematicPoseMemory::quaternion(Vector v,bool normalized){vector(normalized?0x1e1075:0x323816,motion_+0x30,v);}
void NativeTu15KinematicPoseMemory::rotation_component(unsigned n,std::uint32_t v){constexpr std::array<std::uint32_t,12>sites{0x37c644,0x37c64b,0x37c654,0x37c657,0x37c65f,0x37c672,0x37c67d,0x37c680,0x37c687,0x37c692,0x37c6a3,0x37c6a6};scalar(sites.at(n),motion_+0x80+n*4u,v);}
void NativeTu15KinematicPoseMemory::world_origin(Vector v){vector(0x323844,motion_+0xb0,v);}
void NativeTu15KinematicPoseMemory::position_w_one(){scalar(0x32384c,motion_+0x7c,0x3f800000);}
} // namespace halo2
