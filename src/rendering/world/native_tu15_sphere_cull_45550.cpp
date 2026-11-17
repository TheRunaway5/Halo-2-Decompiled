#include "halo2/native_tu15_sphere_cull_45550.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
constexpr std::uint32_t units=0x4c6958u,tags=0x4f0414u,bsp_global=0x4ecc18u,objects=0x4ecbd0u,one=0x46a0d0u,none=0xffffffffu;
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);
    return v;
}
void write(SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t v) {
    std::array<std::byte,4> b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(8*i));
    m.write(at,b);
}
std::int32_t sx16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::int32_t s32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t row_address(std::uint32_t unit){return units+32u*std::uint32_t(sx16(unit));}
// 45563..455FA / 458C9..45955: the unit's resident geometry header.
std::uint32_t geometry(const SourcePhysicsContactMemory& m,std::uint32_t row) {
    const auto model=read(m,row+4u);
    if(model!=none) {
        const auto definition=read(m,read(m,tags)+16u*(model&65535u)+8u);
        return read(m,read(m,definition+0x28u)+92u*((read(m,row+0xcu)>>9)&511u)+0x34u);
    }
    const auto flags=read(m,row),bsp=read(m,bsp_global),packed=read(m,row+0xcu);
    if(!((flags>>12)&1u))return read(m,read(m,bsp+0xa0u)+176u*((packed>>9)&511u)+0x50u);
    const auto instances=read(m,bsp+0x144u);
    const auto definition=std::uint32_t(sx16(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));
    return read(m,read(m,bsp+0x13cu)+200u*definition+0x50u);
}
void set_bit(SourcePhysicsContactMemory& m,std::uint32_t mask,std::uint32_t index) {
    const auto at=mask+4u*(index>>5);
    write(m,at,read(m,at)|(1u<<(index&31u)));
}
}

void native_tu15_sphere_mask_458c0(SourcePhysicsContactMemory& m,std::uint32_t unit,bool ranges,
    std::span<const std::uint32_t> first,std::uint32_t first_count,std::span<const std::uint32_t> second) {
    const auto row=row_address(unit);
    const auto table=geometry(m,row);
    const auto mask=read(m,row+0x14u);
    if(first_count>first.size() || (ranges && (first_count&1u) && first_count>=first.size()))
        throw std::out_of_range("Native458C0 rows escape their array");
    if(s32(first_count)>0 || s32(std::uint32_t(second.size()))>0)write(m,row,read(m,row)|1u);
    for(std::size_t n=0;n<first_count;++n) {
        if(!ranges) {set_bit(m,mask,first[n]&65535u);continue;}
        // 459BB..45A09: both ends index the 16-bit leaf table through the
        // geometry's signed +8 base; the loop index is reread as a signed half.
        const auto leaves=read(m,table+0x34u),base=read(m,table+8u);
        const auto begin=read(m,leaves+2u*(std::uint32_t(sx16(first[n]))+base),2);
        const auto end=read(m,leaves+2u*(std::uint32_t(sx16(first[n+1]))+base),2);
        for(auto index=begin;index<=end;++index)
            set_bit(m,mask,read(m,read(m,table+0x34u)+2u*std::uint32_t(sx16(index)),2));
        ++n;
    }
    for(const auto value:second)set_bit(m,mask,value&65535u);
}

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
__m128 ss(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(s32(bits)));}
__m128 splat(std::uint32_t bits){return _mm_castsi128_ps(_mm_set1_epi32(s32(bits)));}
// MOVSS low, MOVHPS high: {low, 0, high0, high1}.
__m128 low_high(std::uint32_t low,std::uint32_t high0,std::uint32_t high1) {
    return _mm_castsi128_ps(_mm_set_epi32(s32(high1),s32(high0),0,s32(low)));
}
std::uint32_t lane(__m128 v,unsigned n) {
    alignas(16) std::array<std::uint32_t,4> words;_mm_store_ps(reinterpret_cast<float*>(words.data()),v);return words[n];
}
// Two-address forms keep the original destination operand (NaN payloads).
__m128 mulps(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 addps(__m128 a,__m128 b){asm volatile("addps %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 mulss(__m128 a,__m128 b){asm volatile("mulss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 addss(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 subss(__m128 a,__m128 b){asm volatile("subss %1,%0":"+x"(a):"x"(b):"memory");return a;}
__m128 divss(__m128 a,__m128 b){asm volatile("divss %1,%0":"+x"(a):"x"(b):"memory");return a;}
// UCOMISS + LAHF/TEST AH,44/JNP: ordered equality.
bool ordered_equal(__m128 a,__m128 b) {
    unsigned char zero,parity;
    asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetp %1":"=q"(zero),"=q"(parity):"x"(a),"x"(b):"cc","memory");
    return zero&&!parity;
}
// COMISS a,b + JB: below or unordered.
bool below(__m128 a,__m128 b) {
    unsigned char carry;
    asm volatile("comiss %2,%1\n\tsetb %0":"=q"(carry):"x"(a),"x"(b):"cc","memory");
    return carry;
}
using Matrix=std::array<std::uint32_t,13>;
Matrix matrix(const SourcePhysicsContactMemory& m,std::uint32_t at) {
    Matrix out;for(unsigned n=0;n<13;++n)out[n]=read(m,at+4u*n);return out;
}
// 145560(ECX a, EDX b): the composed matrix, in the original packed order.
Matrix compose_145560(const Matrix& a,const Matrix& b) {
    Matrix out{};
    const auto x7=splat(a[0]);
    auto x3=ss(b[0]);
    auto x0=splat(b[10]),x1=splat(b[11]),x2=splat(b[12]);
    x3=mulss(x3,x7);
    const auto x4=low_high(a[1],a[2],a[3]),x5=low_high(a[4],a[5],a[6]),x6=low_high(a[7],a[8],a[9]);
    x0=mulps(x0,x4);x1=mulps(x1,x5);x2=mulps(x2,x6);
    x0=addps(x0,x1);x0=addps(x0,x2);x0=mulps(x0,x7);
    x0=addps(x0,low_high(a[10],a[11],a[12]));
    out[10]=lane(x0,0);out[11]=lane(x0,2);out[12]=lane(x0,3);
    const auto row=[&](std::uint32_t c2,std::uint32_t c1,std::uint32_t c0,unsigned at) {
        auto y2=splat(c2),y1=splat(c1),y0=splat(c0);
        y2=mulps(y2,x6);y1=mulps(y1,x5);y0=mulps(y0,x4);
        y1=addps(y1,y2);y0=addps(y0,y1);
        out[at]=lane(y0,0);out[at+1]=lane(y0,2);out[at+2]=lane(y0,3);
    };
    row(b[9],b[8],b[7],7);row(b[6],b[5],b[4],4);row(b[3],b[2],b[1],1);
    out[0]=lane(x3,0);
    return out;
}
// 145200(ECX matrix, EDX out, ESI point): the point in the matrix's space.
std::array<std::uint32_t,3> inverse_point_145200(const Matrix& t,const std::array<std::uint32_t,3>& point,
    std::uint32_t one_bits) {
    if(ordered_equal(ss(t[0]),_mm_setzero_ps()))return {0,0,0};
    const auto x4=ss(t[0]);auto x3=ss(one_bits);
    auto x0=subss(ss(point[0]),ss(t[10])),x1=subss(ss(point[1]),ss(t[11])),x2=subss(ss(point[2]),ss(t[12]));
    if(!ordered_equal(x4,x3)) {
        x3=divss(x3,x4);
        x0=mulss(x3,x0);x1=mulss(x3,x1);x2=mulss(x3,x2);
    }
    const auto dot=[&](std::uint32_t c3,std::uint32_t c2,std::uint32_t c1) {
        auto y3=mulss(ss(c3),x2);const auto y4=mulss(ss(c2),x1);y3=addss(y3,y4);
        return lane(addss(y3,mulss(ss(c1),x0)),0);
    };
    const auto px=dot(t[3],t[2],t[1]),py=dot(t[6],t[5],t[4]);
    // 1452B4..1452D3: (t9*z + t8*y) + t7*x; MXCSR flags are order-free.
    auto y3=mulss(ss(t[9]),x2);const auto y2=mulss(ss(t[8]),x1);const auto y1=mulss(ss(t[7]),x0);
    y3=addss(y3,y2);y3=addss(y3,y1);
    return {px,py,lane(y3,0)};
}
}
#endif

void native_tu15_sphere_cull_45550(SourcePhysicsContactMemory& m,std::uint32_t unit,
    const std::array<std::uint32_t,3>& center,std::uint32_t radius,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15SphereCullServices& services,NativeTu15SphereCullCoverage* coverage) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    NativeTu15SphereCullCoverage ignored;auto& seen=coverage?*coverage:ignored;
    ++seen.calls;
    const auto row=row_address(unit);
    const auto table=geometry(m,row);
    if(!table) {++seen.no_geometry;return;}
    if(s32(read(m,table+0x30u))>0) {
        // 4562A..45727: the sphere is {center, radius}; 458C0 follows 2E7C40.
        if(!services.query_2e7c40)throw std::invalid_argument("Native45550 requires its 2E7C40 query owner");
        ++seen.queries;
        NativeTu15SphereQueryRows rows;
        services.query_2e7c40(read(m,table+0x2cu),{center[0],center[1],center[2],radius},rows,fp);
        native_tu15_sphere_mask_458c0(m,unit,true,rows.ranges,rows.range_count,rows.leaves);
        return;
    }
    if((fp.control_word&0x3fu)!=0x3fu || (fp.status&0x8080u) || (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
        throw std::invalid_argument("Native45550 requires masked original FP exceptions");
    const auto mask=read(m,row+0x14u);
    if(s32(read(m,table+8u))<=0)return;
    const auto one_bits=read(m,one);
    auto local=center;
    std::uint32_t last_node=none;
    detail::NativeTu15ArithmeticEnvironment environment(fp);
    for(std::uint32_t part=0;;) {
        ++seen.parts;
        const auto word=mask+4u*std::uint32_t(s32(part)>>5),bit=1u<<(part&31u);
        bool mark=false;
        if(read(m,word)&bit) ++seen.already;
        else {
            const auto leaf=read(m,read(m,table+0xcu)+8u*part+4u,2);
            if(leaf==65535u) {++seen.unbounded;mark=true;}
            else {
                const auto sphere=read(m,table+0x14u)+20u*leaf;
                const std::array<std::uint32_t,3> point{read(m,sphere),read(m,sphere+4u),read(m,sphere+8u)};
                const auto model=read(m,row+4u);
                if(model==none)++seen.bsp_parts;
                else if(read(m,read(m,row+0x10u))==none)++seen.no_object;
                else if(const auto node=read(m,sphere+0x10u,1);node==last_node)++seen.same_node;
                else {
                    // BAC30: the object's node bank and its 52-byte row count.
                    const auto object=read(m,read(m,row+0x10u));
                    const auto body=read(m,read(m,read(m,objects)+0x44u)+12u*(object&65535u)+8u);
                    const auto count=std::uint32_t((std::uint64_t(std::uint32_t(sx16(read(m,body+0x114u,2))))*0x4ec4ec4full)>>36u);
                    if(!count)++seen.no_nodes;
                    else {
                        ++seen.transforms;
                        const auto nodes=body+std::uint32_t(sx16(read(m,body+0x116u,2)));
                        const auto definition=read(m,read(m,tags)+16u*(model&65535u)+8u);
                        const auto composed=compose_145560(matrix(m,nodes+0x34u*node),
                            matrix(m,read(m,definition+0x4cu)+96u*node+0x28u));
                        local=inverse_point_145200(composed,center,one_bits);
                        last_node=node;
                    }
                }
                // 45837..45890: (dz*dz + dy*dy) + dx*dx against (r+R)^2.
                const auto dz=subss(ss(local[2]),ss(point[2])),dy=subss(ss(local[1]),ss(point[1])),
                    dx=subss(ss(local[0]),ss(point[0]));
                const auto reach=addss(ss(read(m,sphere+0xcu)),ss(radius));
                const auto distance=addss(addss(mulss(dz,dz),mulss(dy,dy)),mulss(dx,dx));
                if(below(mulss(reach,reach),distance))++seen.misses;
                else {++seen.hits;mark=true;}
            }
        }
        if(mark) {write(m,row,read(m,row)|1u);write(m,word,read(m,word)|bit);}
        ++part;
        if(s32(part)>=s32(read(m,table+8u)))break;
    }
#else
    (void)m;(void)unit;(void)center;(void)radius;(void)fp;(void)services;(void)coverage;
    throw std::invalid_argument("Exact native45550 arithmetic backend unavailable");
#endif
}
}
