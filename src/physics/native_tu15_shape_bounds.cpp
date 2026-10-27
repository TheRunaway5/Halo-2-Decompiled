#include "halo2/native_tu15_shape_bounds.hpp"
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
  std::uint32_t word(std::uint32_t p)const{std::array<std::byte,4>b{};m_.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  __m128 vector(std::uint32_t p)const{
    if(p&15u)throw std::runtime_error("Native TU1.5 shape bounds requires actual aligned vector storage");
    alignas(16)std::array<std::byte,16>b{};m_.read(p,b);__m128 value;std::memcpy(&value,b.data(),16);return value;
  }
  void vector(std::uint32_t pc,std::uint32_t p,__m128 value){
    if(p&15u)throw std::runtime_error("Native TU1.5 shape bounds requires actual aligned vector storage");
    SourcePhysicsContactRecordStore event{pc,p,{},{}};if(o_){event.before.resize(16);m_.read(p,event.before);}
    std::array<std::byte,16>b{};std::memcpy(b.data(),&value,16);m_.write(p,b);
    if(o_){event.written.assign(b.begin(),b.end());o_(event);}
  }
#endif
};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  alignas(16)std::array<std::byte,108>host_{};
  NativeTu15PhysicsFrameFp&fp_;unsigned host_mxcsr_;
public:
  Environment(NativeTu15PhysicsFrameFp&fp,unsigned depth=0):fp_(fp){
    if(depth>7||(fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
      (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
      throw std::runtime_error("Native TU1.5 shape bounds requires its unsupported FP owner");
    host_mxcsr_=_mm_getcsr();
    asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::array<std::byte,28>env{};std::memcpy(env.data(),host_.data(),28);const std::uint16_t empty=0xffffu;
    std::memcpy(env.data(),&fp.control_word,2);std::memcpy(env.data()+4,&fp.status,2);std::memcpy(env.data()+8,&empty,2);
    asm volatile("fldenv %0"::"m"(env):"memory");_mm_setcsr(fp.mxcsr);
  }
  ~Environment(){asm volatile("fnstcw %0":"=m"(fp_.control_word));asm volatile("fnstsw %0":"=a"(fp_.status));fp_.mxcsr=_mm_getcsr();
    asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(host_mxcsr_);}
};
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right));return left;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right));return left;}
__m128 minimum(__m128 left,__m128 right){asm volatile("minps %1,%0":"+x"(left):"x"(right));return left;}
__m128 maximum(__m128 left,__m128 right){asm volatile("maxps %1,%0":"+x"(left):"x"(right));return left;}
__m128 broadcast(std::uint32_t bits){float value;std::memcpy(&value,&bits,4);return _mm_set1_ps(value);}
#endif
void child(const NativeTu15ShapeBoundsServices&s,NativeTu15BoundsShape shape,std::uint32_t owner,
  NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,NativeTu15PhysicsFrameFp&fp){
  if(!s.child_bounds)throw std::runtime_error("Native TU1.5 shape bounds requires its actual child owner");
  s.child_bounds(shape,owner,transform,margin,output,fp);
}
}
void native_tu15_project_box_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsTransform transform,
  std::uint32_t extent,std::uint32_t margin,NativeTu15BoundsOutput output,NativeTu15PhysicsFrameFp&fp,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ShapeBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->box_calls;Memory m(memory,observe);Environment environment(fp);
  const auto dimensions=m.vector(extent);const auto z_basis=m.vector(transform.identity+32u);const auto mask=m.vector(0x461950u);
  auto z=multiply(_mm_shuffle_ps(dimensions,dimensions,0xaau),z_basis);
  const auto y=_mm_shuffle_ps(dimensions,dimensions,0x55u);const auto x=_mm_shuffle_ps(dimensions,dimensions,0u);
  z=add(_mm_and_ps(z,mask),broadcast(margin));
  const auto y_basis=m.vector(transform.identity+16u);auto y_extent=multiply(y,y_basis);
  const auto x_basis=m.vector(transform.identity);auto x_extent=multiply(x,x_basis);
  x_extent=_mm_and_ps(x_extent,mask);y_extent=_mm_and_ps(y_extent,mask);
  const auto total=add(add(x_extent,y_extent),z);m.vector(0x3066feu,output.identity+16u,total);
  const auto sign=m.vector(0x461960u);m.vector(0x30670cu,output.identity,_mm_xor_ps(total,sign));
  const auto first_translation=m.vector(transform.identity+48u);const auto upper=m.vector(output.identity+16u);
  m.vector(0x30671au,output.identity+16u,add(upper,first_translation));
  const auto second_translation=m.vector(transform.identity+48u);const auto lower=m.vector(output.identity);
  m.vector(0x306728u,output.identity,add(lower,second_translation));
#else
  (void)memory;(void)transform;(void)extent;(void)margin;(void)output;(void)fp;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 box bounds backend unavailable");
#endif
}
void native_tu15_convex_box_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
  NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,NativeTu15PhysicsFrameFp&fp,
  unsigned depth,const SourcePhysicsContactRecordObserver&observe,NativeTu15ShapeBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->convex_calls;Memory m(memory,observe);std::uint32_t expanded;
  {Environment environment(fp,depth);asm volatile("flds %0"::"m"(margin):"memory","st");
    const auto radius=m.word(shape.identity+12u);
    asm volatile("fadds %1\n\tfstps %0":"=m"(expanded):"m"(radius):"memory","st");}
  native_tu15_project_box_bounds(memory,transform,shape.identity+16u,expanded,output,fp,observe,stats);
#else
  (void)memory;(void)shape;(void)transform;(void)margin;(void)output;(void)fp;(void)depth;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 convex bounds backend unavailable");
#endif
}
void native_tu15_transformed_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
  NativeTu15BoundsTransform transform,const std::function<std::uint32_t()>&margin_input,
  const std::function<NativeTu15BoundsOutput()>&output_input,NativeTu15BoundsScratch scratch,
  NativeTu15PhysicsFrameFp&fp,const NativeTu15ShapeBoundsServices&services,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ShapeBoundsStats*stats){
  if(stats)++stats->transformed_calls;
  NativeTu15GeometryMemory geometry(memory,scratch.transform,transform.identity,shape.identity+16u,observe);
  native_tu15_compose_affine_geometry(geometry,fp);
  if(!output_input||!margin_input)throw std::runtime_error("Native TU1.5 transformed bounds requires its late source inputs");
  const auto output=output_input();Memory m(memory,observe);const NativeTu15BoundsShape nested{m.word(shape.identity+12u)};
  const auto table=m.word(nested.identity);const auto margin=margin_input();const auto owner=m.word(table+24u);
  child(services,nested,owner,{scratch.transform},margin,output,fp);
}
void native_tu15_forward_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
  NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,
  NativeTu15PhysicsFrameFp&fp,const NativeTu15ShapeBoundsServices&services,NativeTu15ShapeBoundsStats*stats){
  if(stats)++stats->forward_calls;const SourcePhysicsContactRecordObserver none;Memory m(memory,none);
  const NativeTu15BoundsShape nested{m.word(shape.identity+12u)};const auto table=m.word(nested.identity);const auto owner=m.word(table+24u);
  child(services,nested,owner,transform,margin,output,fp);
}
void native_tu15_motion_shape_bounds(SourcePhysicsContactMemory&memory,NativeTu15BoundsShape shape,
  NativeTu15BoundsTransform transform,std::uint32_t margin,NativeTu15BoundsOutput output,
  NativeTu15PhysicsFrameFp&fp,const NativeTu15ShapeBoundsServices&services,
  const SourcePhysicsContactRecordObserver&observe,NativeTu15ShapeBoundsStats*stats){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if(stats)++stats->motion_calls;Memory m(memory,observe);const NativeTu15BoundsShape nested{m.word(shape.identity+0x30u)};
  const auto table=m.word(nested.identity);const auto owner=m.word(table+24u);child(services,nested,owner,transform,margin,output,fp);
  Environment environment(fp);auto delta=_mm_setzero_ps();const auto body=m.word(shape.identity+0x34u);
  if(body){const auto object=m.word(body+8u);if(object){const auto scale=m.word(object+0x150u);const auto motion=m.word(body+0x3cu);
    const auto velocity=m.vector(motion+0x40u);delta=multiply(broadcast(scale),velocity);}}
  const auto upper=m.vector(output.identity+16u);const auto expanded_upper=maximum(add(upper,delta),upper);
  const auto lower=m.vector(output.identity);m.vector(0x2e5ee8u,output.identity+16u,expanded_upper);
  m.vector(0x2e5ef5u,output.identity,minimum(add(lower,delta),lower));
#else
  (void)memory;(void)shape;(void)transform;(void)margin;(void)output;(void)fp;(void)services;(void)observe;(void)stats;
  throw std::runtime_error("Exact native TU1.5 motion bounds backend unavailable");
#endif
}
} // namespace halo2
