#include "halo2/native_tu15_motion_forces.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <utility>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
namespace halo2 {
namespace {
using Vector=NativeTu15PhysicsVector;
using Storage=NativeTu15MotionForceStorage;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// Platform floating-point ownership, not a simulated register machine. Real
// native x87 arithmetic supplies its actual rounding/precision/status, and
// native SIMD supplies its actual exception flags. FNSAVE/FRSTOR preserve the
// unrelated host's complete live x87 state on both success and exceptions.
class Environment {
  alignas(16) std::array<std::byte,108> host_{};
  std::array<std::byte,28> native_{};
  unsigned host_mxcsr_{};
  NativeTu15PhysicsFrameFp&fp_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth):fp_(fp){
    if(depth>5||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
       (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
      throw std::runtime_error("Native TU1.5 motion force requires its unsupported x87/SSE fault owner");
    host_mxcsr_=_mm_getcsr();
    asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::memcpy(native_.data(),host_.data(),native_.size());
    const std::uint16_t empty=0xffff;
    std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);std::memcpy(native_.data()+8,&empty,2);
    asm volatile("fldenv %0"::"m"(native_):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){
    asm volatile("fnstsw %0":"=a"(fp_.status));asm volatile("fnstcw %0":"=m"(fp_.control_word));fp_.mxcsr=_mm_getcsr();
    asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    _mm_setcsr(host_mxcsr_);
  }
};
__m128 simd(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
Vector words(__m128 v){Vector out;_mm_storeu_si128(reinterpret_cast<__m128i*>(out.data()),_mm_castps_si128(v));return out;}
__m128 broadcast(std::uint32_t v){return _mm_castsi128_ps(_mm_set1_epi32(std::bit_cast<std::int32_t>(v)));}
// These native SIMD arithmetic operations retain the source operand order.
// Intrinsics alone permit compilers to commute ADDPS/MULPS and thereby choose
// a different NaN payload when both operands are NaNs. No virtual registers
// or instruction dispatcher are involved: each is one real host operation.
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
void push_seconds(std::uint32_t bits){asm volatile("flds %0"::"m"(bits):"memory","st");}
std::uint32_t force_scale(std::uint32_t inverse_mass){
  std::uint32_t out;asm volatile("fld %%st(0)\n\tfmuls %1\n\tfstps %0":"=m"(out):"m"(inverse_mass):"memory","st");return out;
}
std::uint32_t stored_seconds(){std::uint32_t out;asm volatile("fsts %0":"=m"(out)::"memory");return out;}
void duplicate_seconds(){asm volatile("fld %%st(0)":::"memory","st");}
std::uint32_t damp(std::uint32_t damping,std::uint32_t one,std::uint32_t zero){
  std::uint16_t comparison;std::uint32_t out;
  asm volatile("fmuls %1\n\tfsubrs %2\n\tflds %3\n\tfcomp %%st(1)\n\tfnstsw %0":"=a"(comparison):"m"(damping),"m"(one),"m"(zero):"memory","st");
  // The real x87 comparison says ordered zero>damping. Unordered preserves
  // the original damping NaN; replacing this with std::max changes that path.
  if(!(comparison&0x4100u))asm volatile("fstp %%st(0)\n\tflds %0"::"m"(zero):"memory","st");
  asm volatile("fstps %0":"=m"(out)::"memory","st");return out;
}
void load_zero(std::uint32_t zero){asm volatile("flds %0"::"m"(zero):"memory","st");}
bool has_torque(std::uint32_t torque_w){
  std::uint16_t comparison;asm volatile("flds %1\n\tfucompp\n\tfnstsw %0":"=a"(comparison):"m"(torque_w):"memory","st","st(1)");
  // Original TEST AH,44/JNP skips only ordered equality. Both bits set for
  // unordered, so its even parity takes the torque path as the source does.
  const auto bits=comparison&0x4400u;return bits==0||bits==0x4400u;
}
void angular_impulse(Storage&s,const Vector&impulse){
  const auto z=simd(s.rotation(2)),x=simd(s.rotation(0)),y=simd(s.rotation(1));
  const auto xylo=_mm_shuffle_ps(x,y,0x44),xyhi=_mm_shuffle_ps(x,y,0xee);
  const auto zlo=_mm_shuffle_ps(z,z,0x44),zhi=_mm_shuffle_ps(z,z,0xee),iv=simd(impulse);
  auto local_x=multiply(_mm_shuffle_ps(xylo,zlo,0x88),_mm_shuffle_ps(iv,iv,0));
  const auto local_y=multiply(_mm_shuffle_ps(xylo,zlo,0xdd),_mm_shuffle_ps(iv,iv,0x55));
  const auto world_y=simd(s.rotation(1));local_x=add(local_x,local_y);
  local_x=add(local_x,multiply(_mm_shuffle_ps(xyhi,zhi,0x88),_mm_shuffle_ps(iv,iv,0xaa)));
  const auto world_z=simd(s.rotation(2));const auto local=multiply(simd(s.inverse_inertia()),local_x);
  const auto z_velocity=multiply(world_z,_mm_shuffle_ps(local,local,0xaa));
  const auto y_velocity=multiply(world_y,_mm_shuffle_ps(local,local,0x55));
  const auto x_velocity=multiply(simd(s.rotation(0)),_mm_shuffle_ps(local,local,0));
  const auto previous=simd(s.angular_velocity());const auto delta=add(add(x_velocity,y_velocity),z_velocity);
  s.angular_velocity(words(add(previous,delta)),true);
}
#endif
}
void native_tu15_apply_motion_angular_impulse(Storage&s,const Vector&impulse,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp,depth);angular_impulse(s,impulse);
#else
  (void)s;(void)impulse;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 motion force backend unavailable");
#endif
}
void native_tu15_apply_motion_forces(Storage&s,NativeTu15PhysicsFrameFp&fp,unsigned depth){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp,depth);
  const auto seconds=s.seconds();push_seconds(seconds);const auto force=simd(s.accumulated_force());
  const auto impulse_scale=force_scale(s.inverse_mass());const auto gravity_step=stored_seconds();duplicate_seconds();
  s.linear_velocity(words(add(simd(s.linear_velocity()),multiply(broadcast(impulse_scale),force))),0);
  const auto gravity=simd(s.gravity());
  s.linear_velocity(words(add(simd(s.linear_velocity()),multiply(broadcast(gravity_step),gravity))),1);
  const auto linear_damping=s.linear_damping(),one=s.one(),zero=s.zero();
  const auto damping=damp(linear_damping,one,zero);const auto linear=simd(s.linear_velocity());load_zero(s.zero());
  s.linear_velocity(words(multiply(broadcast(damping),linear)),2);
  const bool torque=has_torque(s.torque_present());s.clear_force();
  if(torque){const auto torque_step=s.seconds();const auto angular_force=simd(s.accumulated_torque());
    angular_impulse(s,words(multiply(broadcast(torque_step),angular_force)));s.clear_torque();}
  const auto angular_damping=s.angular_damping(),angular_one=s.one(),angular_zero=s.zero();
  const auto angular_scale=damp(angular_damping,angular_one,angular_zero);const auto angular=simd(s.angular_velocity());
  s.angular_velocity(words(multiply(broadcast(angular_scale),angular)),false);
#else
  (void)s;(void)fp;(void)depth;throw std::runtime_error("Exact native TU1.5 motion force backend unavailable");
#endif
}
NativeTu15MotionForceMemory::NativeTu15MotionForceMemory(SourcePhysicsContactMemory&m,std::uint32_t motion,std::uint32_t step,std::uint32_t gravity,const SourcePhysicsContactRecordObserver&o)
  :memory_(m),motion_(motion),step_(step),gravity_(gravity),observer_(o){}
std::uint32_t NativeTu15MotionForceMemory::scalar(std::uint32_t p)const{std::array<std::byte,4>b{};memory_.read(p,b);std::uint32_t n{};for(unsigned i=0;i<4;++i)n|=std::to_integer<unsigned>(b[i])<<(i*8u);return n;}
Vector NativeTu15MotionForceMemory::vector(std::uint32_t p)const{std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector out{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)out[k]|=std::to_integer<unsigned>(b[k*4u+i])<<(i*8u);return out;}
void NativeTu15MotionForceMemory::vector(std::uint32_t site,std::uint32_t p,const Vector&v){
  std::array<std::byte,16>b{};for(unsigned k=0;k<4;++k)for(unsigned i=0;i<4;++i)b[k*4u+i]=std::byte(v[k]>>(i*8u));
  SourcePhysicsContactRecordStore store{site,p,{},{}};if(observer_){store.before.resize(16);memory_.read(p,store.before);store.written.assign(b.begin(),b.end());}
  memory_.write(p,b);if(observer_)observer_(store);
}
std::uint32_t NativeTu15MotionForceMemory::seconds()const{return scalar(step_);}
std::uint32_t NativeTu15MotionForceMemory::inverse_mass()const{return scalar(motion_+0x2c);}
std::uint32_t NativeTu15MotionForceMemory::linear_damping()const{return scalar(motion_+0x14);}
std::uint32_t NativeTu15MotionForceMemory::angular_damping()const{return scalar(motion_+0x18);}
std::uint32_t NativeTu15MotionForceMemory::one()const{return scalar(0x46a0d0);}
std::uint32_t NativeTu15MotionForceMemory::zero()const{return scalar(0x46a0e8);}
std::uint32_t NativeTu15MotionForceMemory::torque_present()const{return scalar(motion_+0x10c);}
Vector NativeTu15MotionForceMemory::gravity()const{return vector(gravity_);}
Vector NativeTu15MotionForceMemory::linear_velocity()const{return vector(motion_+0x40);}
Vector NativeTu15MotionForceMemory::angular_velocity()const{return vector(motion_+0x50);}
Vector NativeTu15MotionForceMemory::accumulated_force()const{return vector(motion_+0xf0);}
Vector NativeTu15MotionForceMemory::accumulated_torque()const{return vector(motion_+0x100);}
Vector NativeTu15MotionForceMemory::rotation(unsigned axis)const{if(axis>2)throw std::runtime_error("Unknown native motion rotation axis");return vector(motion_+0x80+axis*16u);}
Vector NativeTu15MotionForceMemory::inverse_inertia()const{return vector(motion_+0xe0);}
void NativeTu15MotionForceMemory::linear_velocity(Vector v,unsigned phase){constexpr std::array<std::uint32_t,3>sites{0x32530e,0x32532c,0x32536e};if(phase>=sites.size())throw std::runtime_error("Unknown native linear force phase");vector(sites[phase],motion_+0x40,v);}
void NativeTu15MotionForceMemory::angular_velocity(Vector v,bool impulse){vector(impulse?0x32515a:0x325400,motion_+0x50,v);}
void NativeTu15MotionForceMemory::clear_force(){vector(0x325382,motion_+0xf0,{});}
void NativeTu15MotionForceMemory::clear_torque(){vector(0x3253c1,motion_+0x100,{});}
} // namespace halo2
