#include "halo2/native_tu15_geometry.hpp"
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif
namespace halo2 {
namespace {
using Vector=NativeTu15GeometryVector;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// These source leaves contain only SSE arithmetic. They neither consume an
// x87 temporary nor alter CW/SW/live x87 values. Own only the real SIMD controls.
class SimdEnvironment {
    NativeTu15PhysicsFrameFp&fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp&fp):fp_(fp),previous_(_mm_getcsr()){
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native TU1.5 geometry requires its unsupported SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 vector(const Vector&v){return _mm_castsi128_ps(_mm_set_epi32(std::bit_cast<std::int32_t>(v[3]),std::bit_cast<std::int32_t>(v[2]),std::bit_cast<std::int32_t>(v[1]),std::bit_cast<std::int32_t>(v[0])));}
Vector words(__m128 v){Vector out;_mm_storeu_si128(reinterpret_cast<__m128i*>(out.data()),_mm_castps_si128(v));return out;}
// Native operations retain both input order and the source's rounding points;
// compiler commutation would select a different payload for paired NaNs.
__m128 multiply(__m128 left,__m128 right){asm volatile("mulps %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addps %1,%0":"+x"(left):"x"(right):"memory");return left;}
#endif
std::uint32_t column(std::uint32_t first,unsigned axis){if(axis>3)throw std::runtime_error("Native TU1.5 geometry column outside its four-column view");return first+axis*16u;}
}
void native_tu15_compose_affine_geometry(NativeTu15GeometryStorage&s,NativeTu15PhysicsFrameFp&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);
    for(unsigned axis=0;axis<4;++axis){
        const auto input=vector(s.source_column(axis));
        const auto basis_z=vector(s.basis_column(2));
        const auto basis_y=vector(s.basis_column(1));
        const auto z_term=multiply(basis_z,_mm_shuffle_ps(input,input,0xaa));
        const auto y_term=multiply(basis_y,_mm_shuffle_ps(input,input,0x55));
        const auto basis_x=vector(s.basis_column(0));
        const auto x_term=multiply(basis_x,_mm_shuffle_ps(input,input,0));
        auto combined=add(x_term,y_term);combined=add(combined,z_term);
        s.destination_column(axis,words(combined),NativeTu15GeometryPublication::composed_column);
    }
    const auto translation=vector(s.basis_column(3));
    const auto current=vector(s.destination_column(3));
    s.destination_column(3,words(add(current,translation)),NativeTu15GeometryPublication::composed_translation);
#else
    (void)s;(void)fp;throw std::runtime_error("Exact native TU1.5 geometry backend unavailable");
#endif
}
void native_tu15_transform_transposed_direction(NativeTu15GeometryStorage&s,NativeTu15PhysicsFrameFp&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);
    const auto basis_z=vector(s.basis_column(2));
    const auto basis_y=vector(s.basis_column(1));
    const auto basis_x=vector(s.basis_column(0));
    const auto input=vector(s.source_column(0));
    const auto z_low=_mm_shuffle_ps(basis_z,basis_z,0x44);
    const auto z_high=_mm_shuffle_ps(basis_z,basis_z,0xee);
    const auto xy_low=_mm_shuffle_ps(basis_x,basis_y,0x44);
    const auto xy_high=_mm_shuffle_ps(basis_x,basis_y,0xee);
    const auto y_axis=_mm_shuffle_ps(xy_low,z_low,0xdd);
    const auto y_term=multiply(y_axis,_mm_shuffle_ps(input,input,0x55));
    const auto x_axis=_mm_shuffle_ps(xy_low,z_low,0x88);
    const auto x_term=multiply(x_axis,_mm_shuffle_ps(input,input,0));
    const auto z_axis=_mm_shuffle_ps(xy_high,z_high,0x88);
    const auto z_term=multiply(z_axis,_mm_shuffle_ps(input,input,0xaa));
    s.destination_column(0,words(add(add(x_term,y_term),z_term)),NativeTu15GeometryPublication::transposed_direction);
#else
    (void)s;(void)fp;throw std::runtime_error("Exact native TU1.5 geometry backend unavailable");
#endif
}
void native_tu15_transform_geometry_position(NativeTu15GeometryStorage&s,NativeTu15PhysicsFrameFp&fp){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    SimdEnvironment environment(fp);
    const auto input=vector(s.source_column(0));
    const auto translation=vector(s.basis_column(3));
    const auto basis_z=vector(s.basis_column(2));
    const auto z_term=multiply(basis_z,_mm_shuffle_ps(input,input,0xaa));
    const auto basis_y=vector(s.basis_column(1));
    const auto translated_z=add(z_term,translation);
    const auto y_term=multiply(basis_y,_mm_shuffle_ps(input,input,0x55));
    const auto basis_x=vector(s.basis_column(0));
    const auto x_term=multiply(basis_x,_mm_shuffle_ps(input,input,0));
    s.destination_column(0,words(add(add(x_term,y_term),translated_z)),NativeTu15GeometryPublication::transformed_position);
#else
    (void)s;(void)fp;throw std::runtime_error("Exact native TU1.5 geometry backend unavailable");
#endif
}
NativeTu15GeometryMemory::NativeTu15GeometryMemory(SourcePhysicsContactMemory&m,std::uint32_t destination,std::uint32_t basis,std::uint32_t source,const SourcePhysicsContactRecordObserver&o):memory_(m),destination_(destination),basis_(basis),source_(source),observer_(o){}
Vector NativeTu15GeometryMemory::vector(std::uint32_t p)const{
    if(p&15u)throw std::runtime_error("Native TU1.5 geometry requires aligned vector storage");
    std::array<std::byte,16>b{};memory_.read_vector(p,b);Vector v{};
    for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)v[n]|=std::to_integer<unsigned>(b[n*4u+k])<<(k*8u);
    return v;
}
Vector NativeTu15GeometryMemory::basis_column(unsigned axis)const{return vector(column(basis_,axis));}
Vector NativeTu15GeometryMemory::source_column(unsigned axis)const{return vector(column(source_,axis));}
Vector NativeTu15GeometryMemory::destination_column(unsigned axis)const{return vector(column(destination_,axis));}
void NativeTu15GeometryMemory::destination_column(unsigned axis,const Vector&v,NativeTu15GeometryPublication phase){
    std::uint32_t site{};switch(phase){case NativeTu15GeometryPublication::composed_column:site=0x2e4522;break;case NativeTu15GeometryPublication::composed_translation:site=0x2e4538;break;case NativeTu15GeometryPublication::transposed_direction:site=0x2e4816;break;case NativeTu15GeometryPublication::transformed_position:site=0x2e46d5;break;}
    const auto p=column(destination_,axis);if(p&15u)throw std::runtime_error("Native TU1.5 geometry requires aligned vector storage");
    std::array<std::byte,16>b{};for(unsigned n=0;n<4;++n)for(unsigned k=0;k<4;++k)b[n*4u+k]=std::byte(v[n]>>(k*8u));
    SourcePhysicsContactRecordStore event{site,p,{},{}};if(observer_){event.before.resize(16);memory_.read(p,event.before);event.written.assign(b.begin(),b.end());}
    memory_.write(p,b);if(observer_)observer_(event);
}
} // namespace halo2
