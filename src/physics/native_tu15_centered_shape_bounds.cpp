#include "halo2/native_tu15_centered_shape_bounds.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
class Memory {
  SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
  Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
  std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t value{};for(unsigned k=0;k<4;++k)value|=std::to_integer<unsigned>(b[k])<<(k*8u);return value;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  __m128 vector(std::uint32_t p)const{if(p&15u)throw std::runtime_error("Native TU1.5 centered bounds requires aligned source storage");alignas(16)std::array<std::byte,16>b{};m_.read(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;}
  void vector(std::uint32_t pc,std::uint32_t p,__m128 value){
    if(p&15u)throw std::runtime_error("Native TU1.5 centered bounds requires aligned output storage");
    SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(16);m_.read(p,event.before);}
    std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);m_.write(p,b);if(o_){event.written.assign(b.begin(),b.end());o_(event);}
  }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth=0):fp_(fp){
    if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||(fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 centered bounds requires its unsupported FP owner");
    host_mxcsr_=_mm_getcsr();asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffff;
    std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right));return left;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right));return left;}
__m128 broadcast(std::uint32_t bits){float value;std::memcpy(&value,&bits,4);return _mm_set1_ps(value);}
#endif
}
void native_tu15_project_centered_box_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsTransform transform,
  std::uint32_t half_extents,std::uint32_t center,std::uint32_t margin,NativeTu15BoundsOutput output,
  NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&observe,NativeTu15CenteredShapeBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->centered_box_calls;Memory m(memory,observe);Environment environment(fp);
  const auto dimensions=m.vector(half_extents);const auto mask=m.vector(0x461950u);const auto z_basis=m.vector(transform.identity+32u);
  auto z=multiply(_mm_shuffle_ps(dimensions,dimensions,0xaau),z_basis);z=add(_mm_and_ps(z,mask),broadcast(margin));
  const auto y_basis=m.vector(transform.identity+16u);auto y=multiply(_mm_shuffle_ps(dimensions,dimensions,0x55u),y_basis);
  const auto x_basis=m.vector(transform.identity);auto x=multiply(_mm_shuffle_ps(dimensions,dimensions,0u),x_basis);
  const auto extent=add(add(_mm_and_ps(x,mask),_mm_and_ps(y,mask)),z);
  m.vector(0x1d6526u,output.identity+16u,extent);const auto sign=m.vector(0x461960u);
  m.vector(0x1d6534u,output.identity,_mm_xor_ps(extent,sign));
  const auto local_center=m.vector(center);const auto translation=m.vector(transform.identity+48u);
  auto center_z=m.vector(transform.identity+32u);center_z=multiply(center_z,_mm_shuffle_ps(local_center,local_center,0xaau));center_z=add(center_z,translation);
  auto center_y=m.vector(transform.identity+16u);center_y=multiply(center_y,_mm_shuffle_ps(local_center,local_center,0x55u));
  auto center_x=m.vector(transform.identity);center_x=multiply(center_x,_mm_shuffle_ps(local_center,local_center,0u));
  const auto upper=m.vector(output.identity+16u);const auto world_center=add(add(center_x,center_y),center_z);
  m.vector(0x1d6577u,output.identity+16u,add(upper,world_center));const auto lower=m.vector(output.identity);
  m.vector(0x1d6581u,output.identity,add(lower,world_center));
#else
  (void)memory;(void)transform;(void)half_extents;(void)center;(void)margin;(void)output;(void)fp;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 centered box bounds backend unavailable");
#endif
}
void native_tu15_centered_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
  NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,
  NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe,NativeTu15CenteredShapeBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->radius_calls;Memory m(memory,observe);std::uint32_t expanded;
  {Environment environment(fp,depth);asm volatile("flds %0"::"m"(margin):"memory","st");const auto radius=m.word(shape.identity+12u);
    asm volatile("fadds %1\n\tfstps %0":"=m"(expanded):"m"(radius):"memory","st");}
  native_tu15_project_centered_box_bounds(memory,transform,shape.identity+16u,shape.identity+32u,expanded,output,fp,observe,stats);
#else
  (void)memory;(void)shape;(void)transform;(void)margin;(void)output;(void)fp;(void)depth;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 centered shape bounds backend unavailable");
#endif
}
} // namespace halo2
