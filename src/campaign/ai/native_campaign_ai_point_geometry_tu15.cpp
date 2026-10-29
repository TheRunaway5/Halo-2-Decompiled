#include "halo2/native_campaign_ai_point_geometry_tu15.hpp"
#include <array>
#include <bit>
#include <stdexcept>
#if defined(__i386__) || defined(__x86_64__)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
    return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(n*8u));
    memory.write(address,event.written);if(observe)observe(event);
}
template<class Sink> void copy_point(const SourcePhysicsContactMemory &memory,
    std::uint32_t source,const Sink &sink,const std::array<std::uint32_t,3> &sites) {
    for(unsigned axis=0;axis<3;++axis)sink(axis,read(memory,source+axis*4u),sites[axis]);
}
template<class Sink> void permute_point(const SourcePhysicsContactMemory &memory,
    std::uint32_t source,std::int16_t axis,std::uint8_t variant,const Sink &sink) {
    const auto offset=(std::uint32_t(std::int32_t(axis))*2u+variant)*6u;
    const auto z_index=std::int16_t(read(memory,0x44acb4u+offset,2));
    const auto z=read(memory,source+std::uint32_t(std::int32_t(z_index))*4u);
    const auto y_index=std::int16_t(read(memory,0x44acb2u+offset,2));
    const auto x_index=std::int16_t(read(memory,0x44acb0u+offset,2));
    const auto y=read(memory,source+std::uint32_t(std::int32_t(y_index))*4u);
    const auto x=read(memory,source+std::uint32_t(std::int32_t(x_index))*4u);
    sink(0,x,0x219063);sink(1,y,0x219065);sink(2,z,0x21906a);
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned previous_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),previous_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::runtime_error("Native campaign point geometry requires its SIMD trap owner");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(previous_);}
};
__m128 scalar(std::uint32_t word){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(word)));}
std::uint32_t word(__m128 value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
__m128 multiply(__m128 left,__m128 right){asm volatile("mulss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 add(__m128 left,__m128 right){asm volatile("addss %1,%0":"+x"(left):"x"(right):"memory");return left;}
__m128 subtract(__m128 left,__m128 right){asm volatile("subss %1,%0":"+x"(left):"x"(right):"memory");return left;}
bool ordered_equal(__m128 left,__m128 right) {
    unsigned char equal{},ordered{};
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetnp %1":"=qm"(equal),"=qm"(ordered):"x"(left),"x"(right):"cc","memory");
    return equal && ordered;
}
template<class Input,class Sink> void transform_point(const SourcePhysicsContactMemory &memory,
    std::uint32_t matrix,const Input &input,const Sink &sink) {
    auto scale=scalar(read(memory,matrix));
    const bool unit_scale=ordered_equal(scale,scalar(read(memory,0x46a0d0u)));
    auto x=scalar(input(0)),y=scalar(input(1)),z=scalar(input(2));
    if(!unit_scale) {
        x=multiply(scale,x);scale=scalar(read(memory,matrix));
        y=multiply(scale,y);scale=scalar(read(memory,matrix));z=multiply(scale,z);
    }
    // Each output is published before the next matrix row is read. Matrix,
    // source and destination may alias; do not snapshot the complete matrix.
    auto z_term=scalar(read(memory,matrix+0x1c)),y_term=scalar(read(memory,matrix+0x10));
    z_term=multiply(z_term,z);y_term=multiply(y_term,y);
    auto sum=add(z_term,y_term);
    sum=add(sum,multiply(scalar(read(memory,matrix+4)),x));
    sum=add(sum,scalar(read(memory,matrix+0x28)));sink(0,word(sum),0x1450d8);
    z_term=scalar(read(memory,matrix+0x20));y_term=scalar(read(memory,matrix+0x14));
    z_term=multiply(z_term,z);y_term=multiply(y_term,y);sum=add(z_term,y_term);
    sum=add(sum,multiply(scalar(read(memory,matrix+8)),x));
    sum=add(sum,scalar(read(memory,matrix+0x2c)));sink(1,word(sum),0x145104);
    z_term=multiply(scalar(read(memory,matrix+0x24)),z);
    y_term=multiply(scalar(read(memory,matrix+0x18)),y);
    const auto x_basis=scalar(read(memory,matrix+0xc));
    sum=add(z_term,y_term);sum=add(sum,multiply(x_basis,x));
    sum=add(sum,scalar(read(memory,matrix+0x30)));sink(2,word(sum),0x145131);
}
template<class Sink> bool resolve_frame(const SourcePhysicsContactMemory &memory,
    std::uint16_t frame,std::uint32_t source,const Sink &sink) {
    if(frame==0xffffu){copy_point(memory,source,sink,{0x21885b,0x218860,0x218866});return true;}
    const auto frames=read(memory,0x505e78u);
    const auto frame_row=frames+std::uint32_t(std::int32_t(std::int16_t(frame)))*16u;
    const auto object=read(memory,frame_row);
    if(object==0xffffffffu){copy_point(memory,source,sink,{0x218840,0x218845,0x21884c});return false;}
    const auto objects=read(memory,0x4ecbd0u);
    const auto object_rows=read(memory,objects+0x44);
    const auto body=read(memory,object_rows+(object&0xffffu)*12u+8u);
    const auto node=std::int16_t(read(memory,frame_row+6,2));
    if(node>=0) {
        const auto node_bytes=std::uint32_t(std::int32_t(std::int16_t(read(memory,body+0x114,2))));
        // The source's unsigned reciprocal division is intentional even for
        // negative authored byte counts. Do not replace it with signed /52.
        const auto count=std::uint32_t((std::uint64_t(node_bytes)*0x4ec4ec4full)>>36u);
        if(std::uint32_t(node)<count) {
            const auto variant=std::uint8_t(read(memory,frame_row+0xb,1));
            const auto axis=std::int16_t(std::int8_t(read(memory,frame_row+0xa,1)));
            std::array<std::uint32_t,3> local{};
            permute_point(memory,source,axis,variant,[&](unsigned n,std::uint32_t value,std::uint32_t){local[n]=value;});
            const auto offset=std::int16_t(read(memory,body+0x116,2));
            const auto matrix=body+std::uint32_t(std::int32_t(offset))+std::uint32_t(node)*0x34u;
            transform_point(memory,matrix,[&](unsigned n){return local[n];},sink);return true;
        }
    }
    copy_point(memory,source,sink,{0x218824,0x218829,0x218831});return false;
}
template<class Sink> void project_point(const SourcePhysicsContactMemory &memory,
    std::uint32_t point,const Sink &sink) {
    const auto frame=std::uint16_t(read(memory,point+0xc,2));
    if(frame==0xffffu)copy_point(memory,point,sink,{0x218b51,0x218b56,0x218b5c});
    else if(!resolve_frame(memory,frame,point,sink))
        copy_point(memory,point,sink,{0x218b3d,0x218b42,0x218b48});
}
template<class Function> auto with_simd(NativeTu15PhysicsFrameFp &fp,Function function) {
    SimdEnvironment environment(fp);return function();
}
#endif
}
void native_campaign_ai_permute_point_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t source,std::uint32_t destination,std::int16_t axis,std::uint8_t variant,
    const SourcePhysicsContactRecordObserver &observe) {
    permute_point(memory,source,axis,variant,[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,destination+n*4u,value);});
}
void native_campaign_ai_transform_point_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t matrix,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    with_simd(fp,[&]{transform_point(memory,matrix,[&](unsigned n){return read(memory,source+n*4u);},
        [&](unsigned n,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,destination+n*4u,value);});});
#else
    (void)memory;(void)matrix;(void)source;(void)destination;(void)fp;(void)observe;
    throw std::runtime_error("Exact native campaign point geometry backend unavailable");
#endif
}
bool native_campaign_ai_resolve_frame_tu15(SourcePhysicsContactMemory &memory,
    std::uint16_t frame,std::uint32_t source,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return with_simd(fp,[&]{return resolve_frame(memory,frame,source,
        [&](unsigned n,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,destination+n*4u,value);});});
#else
    (void)memory;(void)frame;(void)source;(void)destination;(void)fp;(void)observe;
    throw std::runtime_error("Exact native campaign point geometry backend unavailable");
#endif
}
void native_campaign_ai_project_point_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t point,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    with_simd(fp,[&]{project_point(memory,point,[&](unsigned n,std::uint32_t value,std::uint32_t site){publish(memory,observe,site,destination+n*4u,value);});});
#else
    (void)memory;(void)point;(void)destination;(void)fp;(void)observe;
    throw std::runtime_error("Exact native campaign point geometry backend unavailable");
#endif
}
std::uint32_t native_campaign_ai_point_distance_squared_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t first,std::uint32_t second,NativeTu15PhysicsFrameFp &fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return with_simd(fp,[&]{
        const auto first_frame=read(memory,first+0xc,2),second_frame=read(memory,second+0xc,2);
        __m128 x,y,z;
        if(first_frame==second_frame) {
            x=scalar(read(memory,second));y=scalar(read(memory,second+4));z=scalar(read(memory,second+8));
            x=subtract(x,scalar(read(memory,first)));y=subtract(y,scalar(read(memory,first+4)));z=subtract(z,scalar(read(memory,first+8)));
        }else {
            std::array<std::uint32_t,3> a{},b{};
            project_point(memory,first,[&](unsigned n,std::uint32_t v,std::uint32_t){a[n]=v;});
            project_point(memory,second,[&](unsigned n,std::uint32_t v,std::uint32_t){b[n]=v;});
            // The unequal-frame source assigns its x/y/z differences to a
            // different reduction order. Keep that visible rounding boundary.
            y=subtract(scalar(b[0]),scalar(a[0]));z=subtract(scalar(b[1]),scalar(a[1]));x=subtract(scalar(b[2]),scalar(a[2]));
        }
        const auto z_squared=multiply(z,z);x=multiply(x,x);
        const auto first_sum=add(x,z_squared);
        const auto y_squared=multiply(y,y);
        return word(add(first_sum,y_squared));
    });
#else
    (void)memory;(void)first;(void)second;(void)fp;
    throw std::runtime_error("Exact native campaign point geometry backend unavailable");
#endif
}
std::array<std::uint32_t,3> native_campaign_ai_project_point_value_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t point,NativeTu15PhysicsFrameFp &fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    return with_simd(fp,[&]{std::array<std::uint32_t,3> result{};
        project_point(memory,point,[&](unsigned n,std::uint32_t value,std::uint32_t){result[n]=value;});
        return result;
    });
#else
    (void)memory;(void)point;(void)fp;
    throw std::runtime_error("Exact native campaign point geometry backend unavailable");
#endif
}
}
