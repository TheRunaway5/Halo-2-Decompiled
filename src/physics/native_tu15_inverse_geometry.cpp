#include "halo2/native_tu15_inverse_geometry.hpp"
#include <cstring>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class Environment {
  NativeTu15PhysicsFrameFp&fp_;unsigned prior_;
public:
  explicit Environment(NativeTu15PhysicsFrameFp&fp):fp_(fp),prior_(_mm_getcsr()){
    if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::runtime_error("Native TU1.5 inverse geometry requires its unsupported SIMD exception owner");
    _mm_setcsr(fp.mxcsr);
  }
  ~Environment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(prior_);}
};
__m128 value(const NativeTu15GeometryVector&v){__m128 result;std::memcpy(&result,v.data(),16);return result;}
NativeTu15GeometryVector words(__m128 v){NativeTu15GeometryVector result;std::memcpy(result.data(),&v,16);return result;}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subps %1,%0":"+x"(left):"x"(right):"memory");return left;}
struct Transpose {__m128 x,y,z;};
Transpose transpose(__m128 x,__m128 y,__m128 z){
  const auto xy_low=_mm_shuffle_ps(x,y,0x44),xy_high=_mm_shuffle_ps(x,y,0xee);
  const auto zz_low=_mm_shuffle_ps(z,z,0x44),zz_high=_mm_shuffle_ps(z,z,0xee);
  return{_mm_shuffle_ps(xy_low,zz_low,0x88),_mm_shuffle_ps(xy_low,zz_low,0xdd),_mm_shuffle_ps(xy_high,zz_high,0x88)};
}
#endif
}
void native_tu15_compose_inverse_affine_geometry(NativeTu15GeometryStorage&s,NativeTu15PhysicsFrameFp&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  Environment environment(fp);
  const auto z=value(s.basis_column(2)),x=value(s.basis_column(0)),y=value(s.basis_column(1));const auto basis=transpose(x,y,z);
  for(unsigned n=0;n<4;++n){
    const auto source=value(s.source_column(n));
    const auto z_term=multiply(basis.z,_mm_shuffle_ps(source,source,0xaa));
    const auto y_term=multiply(basis.y,_mm_shuffle_ps(source,source,0x55));
    const auto x_term=multiply(basis.x,_mm_shuffle_ps(source,source,0));
    s.destination_column(n,words(add(add(x_term,y_term),z_term)),NativeTu15GeometryPublication::composed_column);
  }
  const auto next_z=value(s.basis_column(2)),translation=value(s.basis_column(3));
  const auto next_x=value(s.basis_column(0)),next_y=value(s.basis_column(1));const auto next_basis=transpose(next_x,next_y,next_z);
  const auto y_term=multiply(next_basis.y,_mm_shuffle_ps(translation,translation,0x55));
  const auto previous=value(s.destination_column(3));
  const auto x_term=multiply(next_basis.x,_mm_shuffle_ps(translation,translation,0));
  const auto z_term=multiply(next_basis.z,_mm_shuffle_ps(translation,translation,0xaa));
  s.destination_column(3,words(subtract(previous,add(add(x_term,y_term),z_term))),NativeTu15GeometryPublication::composed_translation);
#else
  (void)s;(void)fp;throw std::runtime_error("Exact native TU1.5 inverse geometry backend unavailable");
#endif
}
NativeTu15InverseGeometryMemory::NativeTu15InverseGeometryMemory(SourcePhysicsContactMemory&m,std::uint32_t destination,
  std::uint32_t basis,std::uint32_t source,const SourcePhysicsContactRecordObserver&o)
  :memory_(m),destination_(destination),basis_(basis),source_(source),observer_(o){}
NativeTu15GeometryVector NativeTu15InverseGeometryMemory::vector(std::uint32_t p)const{
  if(p&15u)throw std::runtime_error("Native TU1.5 inverse geometry requires aligned vector storage");
  std::array<std::byte,16>b{};memory_.read_vector(p,b);NativeTu15GeometryVector result{};
  for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)result[n]|=std::to_integer<unsigned>(b[n*4+k])<<(k*8u);return result;
}
NativeTu15GeometryVector NativeTu15InverseGeometryMemory::basis_column(unsigned n)const{return vector(basis_+n*16u);}
NativeTu15GeometryVector NativeTu15InverseGeometryMemory::source_column(unsigned n)const{return vector(source_+n*16u);}
NativeTu15GeometryVector NativeTu15InverseGeometryMemory::destination_column(unsigned n)const{return vector(destination_+n*16u);}
void NativeTu15InverseGeometryMemory::destination_column(unsigned n,const NativeTu15GeometryVector&value,NativeTu15GeometryPublication publication){
  const auto p=destination_+n*16u;if(p&15u)throw std::runtime_error("Native TU1.5 inverse geometry requires aligned vector storage");
  const auto pc=publication==NativeTu15GeometryPublication::composed_column?0x2e45d0u:
    publication==NativeTu15GeometryPublication::composed_translation?0x2e463du:0u;
  if(!pc)throw std::runtime_error("Unbound native inverse geometry publication");
  SourcePhysicsContactRecordStore event{pc,p,{},{}};if(observer_){event.before.resize(16);memory_.read(p,event.before);}
  std::array<std::byte,16>b{};for(unsigned lane=0;lane<4;++lane)for(unsigned k=0;k<4;++k)b[lane*4+k]=std::byte(value[lane]>>(k*8u));
  memory_.write(p,b);if(observer_){event.written.assign(b.begin(),b.end());observer_(event);}
}
} // namespace halo2
