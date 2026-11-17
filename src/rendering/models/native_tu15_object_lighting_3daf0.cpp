#include "halo2/native_tu15_object_lighting_3daf0.hpp"
#include "halo2/native_tu15_function_evaluation_extended.hpp"
#include <array>
#include <bit>
#include <cstring>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#include <xmmintrin.h>
#endif

// Native TU1.5 3DAF0 object lighting. Ordinary C++ control flow over the
// current banks; every SSE scalar operation keeps the original operand order
// (NaN payloads and MXCSR flags) and every x87 sequence is the original one
// (precision control, sticky and C1 status), with its own operands in host
// memory. Comments give the original instruction ranges.
namespace halo2 {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
using u32=std::uint32_t;
using i32=std::int32_t;
constexpr u32 none=0xffffffffu;
// Original constants (TU1.5 .rdata), by address.
constexpr u32 k_zero=0x00000000u,k_one=0x3f800000u,k_two=0x40000000u,k_half=0x3f000000u,
    k_epsilon=0x38d1b717u/*46A0EC 1e-4*/,k_400=0x43c80000u,k_200=0x43480000u,k_100=0x42c80000u,
    k_50=0x42480000u,k_10=0x41200000u,k_near=0x3b23d70bu/*46A6E0*/,k_speed=0x3e19999au/*44A8A0*/,
    k_speed_scale=0x3d09382du/*46AA30*/,k_moved=0x3d4ccccdu/*46A12C*/,k_hundredth=0x3c23d70au/*46A18C*/,
    k_two32=0x4f800000u/*46A0C8*/,k_255th=0x3b808081u/*46A0C4*/,k_255=0x437f0000u/*46A148*/,
    k_lowest=0xff7fffffu/*44AAC8*/,k_1000=0x447a0000u/*46A1DC*/,k_d2c0=0x3db851ecu/*46A4C0*/,
    k_d2bc=0x3df5c28fu/*46A4BC*/,k_04=0x3ecccccdu/*46A240*/,k_degrees=0x3c8efa35u/*46A164*/,
    k_118=0x3f970a3du/*46A824*/,k_075=0x3f400000u/*46A13C*/,k_01=0x3dcccccdu/*46A178*/,
    k_06=0x3f19999au/*456914*/,k_07=0x3f333333u/*46A22C*/,k_l114=0x3de978d5u/*46A1CC*/,
    k_l587=0x3f1645a2u/*46A1C8*/,k_l299=0x3e991687u/*46A1C4*/,k_m075=0xbf400000u/*46A7A4*/,
    k_m1=0xbf800000u/*46A0DC*/,k_011=0x3de147aeu/*46A7A0*/,k_059=0x3f170a3du/*46A79C*/,
    k_03=0x3e99999au/*456918*/,k_14=0x3fb33333u/*46A798*/,k_dot=0x3f7ff972u/*46A188*/,
    k_005=0x3d4ccccdu/*46A12C*/;
constexpr std::uint64_t k_d4c8=0xbff41b2f80000000ull/*46A4C8*/,k_d0f0=0/*46A0F0*/,
    k_d650=0x3fd0000000000000ull/*46A650 .25*/,k_d738=0x3fe8000000000000ull/*46A738 .75*/;

struct Bank {
    SourcePhysicsContactMemory& m;
    u32 r32(u32 a)const{std::array<std::byte,4>b;m.read(a,b);return u32(b[0])|u32(b[1])<<8|u32(b[2])<<16|u32(b[3])<<24;}
    u32 r16(u32 a)const{std::array<std::byte,2>b;m.read(a,b);return u32(b[0])|u32(b[1])<<8;}
    u32 r8(u32 a)const{std::array<std::byte,1>b;m.read(a,b);return u32(b[0]);}
    void w32(u32 a,u32 v){const std::array b{std::byte(v),std::byte(v>>8),std::byte(v>>16),std::byte(v>>24)};m.write(a,b);}
    void w16(u32 a,u32 v){const std::array b{std::byte(v),std::byte(v>>8)};m.write(a,b);}
    void w8(u32 a,u32 v){const std::array b{std::byte(v)};m.write(a,b);}
    void copy(u32 to,u32 from,unsigned words){for(unsigned n=0;n<words;++n)w32(to+4*n,r32(from+4*n));}
    std::array<u32,3> f3(u32 a)const{return{r32(a),r32(a+4),r32(a+8)};}
    void f3(u32 a,const std::array<u32,3>& v){w32(a,v[0]);w32(a+4,v[1]);w32(a+8,v[2]);}
    // Object row body (ECX=[4ECBD0]+44; [rows+index*12+8]).
    u32 body(u32 object)const{return r32(r32(r32(0x4ecbd0u)+0x44u)+(object&65535u)*12u+8u);}
    u32 definition(u32 tag)const{return r32(r32(0x4f0414u)+(tag&65535u)*16u+8u);}
    u32 record(u32 token)const{return r32(r32(0x515f14u)+0x44u)+((token&65535u)<<8);}
};
i32 sx16(u32 v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
i32 s32(u32 v){return std::bit_cast<i32>(v);}

// SSE scalar arithmetic on raw single bits, two-address order.
__m128 X(u32 b){return _mm_castsi128_ps(_mm_cvtsi32_si128(s32(b)));}
u32 B(__m128 v){return u32(_mm_cvtsi128_si32(_mm_castps_si128(v)));}
u32 mul(u32 a,u32 b){auto x=X(a);asm volatile("mulss %1,%0":"+x"(x):"x"(X(b)):"memory");return B(x);}
u32 add(u32 a,u32 b){auto x=X(a);asm volatile("addss %1,%0":"+x"(x):"x"(X(b)):"memory");return B(x);}
u32 sub(u32 a,u32 b){auto x=X(a);asm volatile("subss %1,%0":"+x"(x):"x"(X(b)):"memory");return B(x);}
u32 dvd(u32 a,u32 b){auto x=X(a);asm volatile("divss %1,%0":"+x"(x):"x"(X(b)):"memory");return B(x);}
// COMISS a,b; JA.
bool gt(u32 a,u32 b){unsigned char r;asm volatile("comiss %2,%1;seta %0":"=qm"(r):"x"(X(a)),"x"(X(b)):"cc","memory");return r;}
u32 cvt(i32 i){auto x=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(x):"r"(i):"memory");return B(x);}
// The clamp shape used throughout: (lo > v) ? lo : ((v > hi) ? hi : v).
u32 clamp(u32 v,u32 lo,u32 hi){return gt(lo,v)?lo:(gt(v,hi)?hi:v);}

// x87 blocks: the original instruction sequences, empty stack in and out.
#define H2_X87_CLOBBERS "st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)","cc","memory"
// fld [p+8]; fld [p+4]; fld [p]; length; fstp st3; fstp; fstp: ST0=|p|.
#define H2_LEN3(p) "flds 8(" p ")\n\tflds 4(" p ")\n\tflds (" p ")\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\t" \
    "fld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfld %%st(3)\n\tfmul %%st(4),%%st\n\t" \
    "faddp %%st,%%st(1)\n\tfsqrt\n\tfstp %%st(3)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\t"
// fst [out]; fabs; fld 1e-4; fcomip; fstp: AL=(1e-4 > |ST0|).
#define H2_SMALL(out) "fsts " out "\n\tfabs\n\tflds %[eps]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[small]\n\t"
const u32 epsilon_bits=k_epsilon;
// 30BD0 head: length, rounded length and its small test.
bool length_small(const std::array<u32,3>& v,u32& rounded) {
    unsigned char small;
    asm volatile(H2_LEN3("%[v]") H2_SMALL("%[r]")
        :[r]"=m"(rounded),[small]"=qm"(small):[v]"r"(v.data()),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
    return small;
}
// 30BD0: normalize in place; returns XMM0.
u32 normalize(Bank& b,u32 at) {
    const auto v=b.f3(at);u32 length;
    if(length_small(v,length))return k_zero;
    const auto inverse=dvd(k_one,length);
    b.w32(at,mul(b.r32(at),inverse));b.w32(at+4,mul(inverse,b.r32(at+4)));b.w32(at+8,mul(inverse,b.r32(at+8)));
    return length;
}
// 1346C0: packed RGB to three floats.
std::array<u32,3> unpack(u32 packed) {
    std::array<u32,3> out;
    for(unsigned n=0;n<3;++n) {
        const i32 channel=i32((packed>>(16-8*n))&255u);
        // FILD; the 46A0C8 bias applies to a negative dword only.
        asm volatile("fildl %[c]\n\tfmuls %[k]\n\tfstps %[o]":[o]"=m"(out[n]):[c]"m"(channel),[k]"m"(k_255th):H2_X87_CLOBBERS);
    }
    return out;
}
// 1349E0: three floats to 0x00RRGGBB (FISTP under the current rounding).
u32 pack(const std::array<u32,3>& c) {
    const u32 scale=k_255;std::array<i32,3> i;
    asm volatile("flds (%[c])\n\tflds 4(%[c])\n\tflds 8(%[c])\n\tflds %[k]\n\t"
        "fmul %%st,%%st(3)\n\tfmul %%st,%%st(2)\n\tfmulp %%st,%%st(1)\n\t"
        "fistpl (%[i])\n\tfistpl 4(%[i])\n\tfistpl 8(%[i])"
        ::[c]"r"(c.data()),[k]"m"(scale),[i]"r"(i.data()):H2_X87_CLOBBERS);
    return (u32(i[0])&255u)|((u32(i[1])&255u)<<8)|((u32(i[2])&255u)<<16);
}

class Arithmetic {
    alignas(16) std::array<std::byte,108> host_{};
    NativeTu15PhysicsFrameFp& fp_;unsigned host_mxcsr_;bool active_{};
    void enter() {
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::array<std::byte,28> env{};std::memcpy(env.data(),host_.data(),env.size());const std::uint16_t empty=0xffffu;
        std::memcpy(env.data(),&fp_.control_word,2);std::memcpy(env.data()+4,&fp_.status,2);std::memcpy(env.data()+8,&empty,2);
        asm volatile("fldenv %0"::"m"(env):"memory");host_mxcsr_=_mm_getcsr();_mm_setcsr(fp_.mxcsr);active_=true;
    }
public:
    explicit Arithmetic(NativeTu15PhysicsFrameFp& fp):fp_(fp) {
        if(fp.control_word!=0x027fu || (fp.status&0x8080u) || (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
            throw std::invalid_argument("Native TU1.5 object lighting requires the original 027F control word and masked exceptions");
        enter();
    }
    // Publish this environment to `fp` for a nested owner, then resume it.
    void leave() {
        if(!active_)return;
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(fp_.control_word),"=m"(fp_.status));fp_.mxcsr=_mm_getcsr();
        asm volatile("frstor %0"::"m"(host_):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        _mm_setcsr(host_mxcsr_);active_=false;
    }
    void resume(){if(!active_)enter();}
    ~Arithmetic(){leave();}
};

struct Context {
    Bank b;NativeTu15PhysicsFrameFp& fp;Arithmetic& arithmetic;
    const NativeTu15ObjectLightingServices& services;NativeTu15ObjectLightingStats* stats;
    void count(std::uint64_t NativeTu15ObjectLightingStats::*field){if(stats)++(stats->*field);}
};

// BCFB0(ECX object): its root.
u32 root(const Bank& b,u32 object) {
    if(object==none)return none;
    u32 result;
    do {result=object;object=b.r32(b.body(result)+0x14u);} while(object!=none);
    return result;
}
// BCDF0(EAX object, mask): the body when the object's type bit is in mask.
u32 typed_body(const Bank& b,u32 object,u32 mask) {
    if(object==none)return 0;
    const auto pool=b.r32(0x4ecbd0u),index=object&65535u;
    if(s32(index)>=s32(b.r32(pool+0x38u)))return 0;
    const auto header=b.r32(pool+0x24u)*index+b.r32(pool+0x44u);
    const auto salt=sx16(b.r16(header));
    if(!salt || salt!=(s32(object)>>16))return 0;
    if(!(mask&(1u<<(b.r8(header+3u)&31u))))return 0;
    return b.r32(header+8u);
}
// 16EA60(ECX array, EDX index): the next used index at or after `index`.
u32 next_datum(const Bank& b,u32 array,u32 index) {
    if(s32(index)<0)return none;
    const auto extent=b.r32(array+0x38u);
    if(s32(index)>=s32(extent))return none;
    const auto bits=b.r32(array+0x48u);
    do {
        if(b.r32(bits+u32(s32(index)>>5)*4u)&(1u<<(index&31u)))return index;
        ++index;
    } while(s32(index)<s32(extent));
    return none;
}
// 16E6A0(EDX array): allocate one datum.
u32 allocate_datum(Bank& b,u32 array) {
    auto index=b.r32(array+0x34u);const auto extent=b.r32(array+0x38u);
    bool found=false;
    if(s32(index)<s32(extent)) {
        const auto bits=b.r32(array+0x48u);
        for(;;) {
            if(!(b.r32(bits+u32(s32(index)>>5)*4u)&(1u<<(index&31u)))){found=index!=none;break;}
            ++index;
            if(s32(index)>=s32(b.r32(array+0x38u)))break;
        }
    }
    if(!found) {
        if(s32(extent)>=s32(b.r32(array+0x20u)) || extent==none)return none;
        index=extent;
    }
    const auto record=b.r32(array+0x24u)*index+b.r32(array+0x44u);
    const auto word=b.r32(array+0x48u)+u32(s32(index)>>5)*4u;
    b.w32(word,b.r32(word)|(1u<<(index&31u)));
    b.w32(array+0x3cu,b.r32(array+0x3cu)+1u);
    b.w32(array+0x34u,index+1u);
    if(!(s32(b.r32(array+0x38u))>s32(index)))b.w32(array+0x38u,index+1u);
    const auto stride=b.r32(array+0x24u);
    for(u32 n=0;n<(stride>>2);++n)b.w32(record+4u*n,0);
    for(u32 n=0;n<(stride&3u);++n)b.w8(record+(stride&~3u)+n,0);
    b.w16(record,b.r16(array+0x40u));
    b.w16(array+0x40u,b.r16(array+0x40u)+1u);
    if(b.r16(array+0x40u)==0xffffu)b.w16(array+0x40u,0x8000u);
    return (u32(sx16(b.r16(record)))<<16)|index;
}
// 2C390(EAX): default lighting block of a fresh cache record (+44).
void default_lighting(Bank& b,u32 at) {
    std::array<u32,3> values;
    asm volatile("fldl %[a]\n\tfcos\n\t"
        "fldl %[z]\n\tfcos\n\tfmul %%st(1),%%st\n\tfstps (%[v])\n\t"
        "fldl %[z]\n\tfsin\n\tfmul %%st(1),%%st\n\tfstps 4(%[v])\n\tfstp %%st(0)\n\t"
        "fldl %[a]\n\tfsin\n\tfstps 8(%[v])"
        ::[a]"m"(k_d4c8),[z]"m"(k_d0f0),[v]"r"(values.data()):H2_X87_CLOBBERS);
    b.w8(at+0x74u,0);b.w32(at+4u,0);b.w8(at+0x77u,0);b.w8(at+0x76u,0);b.w32(at,none);b.w32(at+8u,none);
    for(const auto offset:{0x10u,0x48u,0x14u,0x4cu,0x18u,0x50u})b.w32(at+offset,k_d2c0);
    b.w32(at+0x30u,k_d2bc);b.w32(at+0x34u,k_d2bc);b.w32(at+0x38u,k_d2bc);b.w32(at+0x28u,k_04);b.w32(at+0x2cu,k_04);
    b.w32(at+0x3cu,values[0]);b.w32(at+0x40u,values[1]);b.w32(at+0x44u,values[2]);
    b.w32(at+0x54u,sub(k_zero,b.r32(at+0x3cu)));b.w32(at+0x58u,sub(k_zero,b.r32(at+0x40u)));
    b.w32(at+0x5cu,sub(k_zero,b.r32(at+0x44u)));
    b.copy(at+0x1cu,at+0x3cu,3);b.w16(at+0x60u,0xffffu);
}
// 3DD00(object, forced): the record's four packed change colors.
void publish_colors(Context& c,u32 object,bool forced) {
    auto& b=c.b;const auto body=b.body(object),token=b.r32(body+0xccu);
    if(token==none)return;
    if(!forced && !(b.r8(b.definition(b.r32(body))+0x1cu)&1u))return;
    c.count(&NativeTu15ObjectLightingStats::color_publications);
    const auto record=b.record(token);
    b.w8(record+0xb8u,0);
    for(u32 n=0;n<4;++n) {
        // BCD80(EAX object, EDI n, ESI out).
        const auto row=b.body(object);
        const auto colors=row+u32(sx16(b.r16(row+0x126u)));
        const auto count=i32((u32(sx16(b.r16(row+0x124u)))/12u)>>1);
        if(s32(n)<0 || s32(n)>=count)continue;
        const auto color=b.f3(colors+(u32(count)+n)*12u);
        b.w32(record+0xa8u+4u*n,pack(color));
        b.w8(record+0xb8u,(b.r8(record+0xb8u)+1u)&255u);
    }
}
}  // namespace

std::uint32_t native_tu15_object_lighting_acquire_3ddc0(SourcePhysicsContactMemory& memory,std::uint32_t object,
    NativeTu15PhysicsFrameFp& fp,NativeTu15ObjectLightingStats* stats) {
    Arithmetic arithmetic(fp);
    static const NativeTu15ObjectLightingServices no_services;
    Context c{{memory},fp,arithmetic,no_services,stats};
    auto& b=c.b;c.count(&NativeTu15ObjectLightingStats::acquisitions);
    const auto body=b.body(object),cached=b.r32(body+0xccu),cache=b.r32(0x515f14u);
    u32 token=none;bool fresh=false;
    if(cached!=none && b.r32(b.r32(cache+0x44u)+((cached&65535u)<<8)+4u)==object)token=cached;
    if(token==none) {
        token=allocate_datum(b,cache);
        if(token!=none)c.count(&NativeTu15ObjectLightingStats::allocations);
        else {
            // 3DE2A..3DEF4: evict the record rendered longest ago.
            auto index=next_datum(b,cache,0);
            if(index==none)return none;
            const auto rows=b.r32(cache+0x44u);
            auto handle=(u32(sx16(b.r16(rows+b.r32(cache+0x24u)*index)))<<16)|index;
            if(handle==none)return none;
            auto best=k_lowest;
            do {
                const auto age=cvt(s32(b.r32(0x4c6854u)-b.r32(rows+((handle&65535u)<<8)+12u)));
                const auto value=gt(k_zero,age)?k_1000:age;
                if(gt(value,best)){best=value;token=handle;}
                index=next_datum(b,cache,(handle&65535u)+1u);handle=none;
                if(index!=none)handle=(u32(sx16(b.r16(rows+b.r32(cache+0x24u)*index)))<<16)|index;
            } while(handle!=none);
            if(token==none)return none;
            c.count(&NativeTu15ObjectLightingStats::evictions);
        }
        fresh=true;
    }
    const auto record=b.r32(cache+0x44u)+((token&65535u)<<8);
    if(fresh) {
        b.w32(record+4u,object);b.w32(record+8u,none);b.w32(record+12u,none);b.w8(record+2u,0);b.w8(record+3u,0);
        default_lighting(b,record+0x44u);
        b.w32(record+0x44u,object);b.w32(record+0x48u,0xc08e0u);b.w32(record+0xbcu,none);
        // 3DF2E..3DF8C: four copies of the body's +118/+11A region bank.
        const auto current=b.body(object);
        const auto from=current+u32(sx16(b.r16(current+0x11au)));
        const auto bytes=sx16(b.r16(current+0x118u))/10;
        if(bytes<0)throw std::runtime_error("Native TU1.5 object lighting region bank has a negative size");
        for(u32 copy=0,to=record+0xc0u;copy<4;++copy,to+=16u) {
            for(u32 n=0;n<u32(bytes)/4u;++n)b.w32(to+4u*n,b.r32(from+4u*n));
            for(u32 n=u32(bytes)&~3u;n<u32(bytes);++n)b.w8(to+n,b.r8(from+n));
        }
        b.w32(body+0xccu,token);
        publish_colors(c,object,true);
    }
    return token;
}

namespace {
// 3E4D0(ESI current, target, XMM2 step): step a packed RGB color.
void step_color(Bank& b,u32 current,u32 target,u32 step) {
    auto from=unpack(b.r32(current));const auto to=unpack(b.r32(target));
    const auto lower=sub(k_zero,step);
    for(unsigned n=0;n<2;++n) {
        auto delta=sub(to[n],from[n]);
        if(gt(lower,delta))delta=lower;else if(gt(delta,step))delta=step;
        from[n]=add(delta,from[n]);
    }
    from[2]=add(clamp(sub(to[2],from[2]),lower,step),from[2]);
    b.w32(current,pack(from));
}
// 32CCD8 for |x| < 1 (the 3E590 caller admits |x| < 46A188 only): acos.
// 3E590 (ESI current, EAX target, step): rotate the current incident toward
// the target by at most step*angle and move its length by at most step.
void step_incident(Context& c,u32 current,u32 target,u32 step) {
    auto& b=c.b;
    const auto cv=b.f3(current),tv=b.f3(target);
    u32 current_length,target_length;
    {
        // 3E593..3E5D5
        u32 rounded;unsigned char small;
        asm volatile("flds 8(%[v])\n\tflds 4(%[v])\n\tflds (%[v])\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\t"
            "fld %%st(2)\n\tfmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfld %%st(3)\n\tfmul %%st(4),%%st\n\t"
            "faddp %%st,%%st(1)\n\tfsqrt\n\tfstps %[r]\n\tfstp %%st(0)\n\tfstp %%st(0)\n\tfstp %%st(0)\n\t"
            "flds %[r]\n\tflds %[eps]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[small]"
            :[r]"=m"(rounded),[small]"=qm"(small):[v]"r"(cv.data()),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
        current_length=small?k_epsilon:(gt(rounded,k_10)?k_10:rounded);
    }
    {
        // 3E5F6..3E67C: the target length with an SSE x*x term, recomputed.
        const auto square=mul(tv[0],tv[0]);unsigned char small,large{};u32 rounded{};
        asm volatile("flds 8(%[v])\n\tflds 4(%[v])\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(2)\n\t"
            "fmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfadds %[q]\n\tfsqrt\n\tfstp %%st(2)\n\tfstp %%st(0)\n\t"
            "flds %[eps]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[small]"
            :[small]"=qm"(small):[v]"r"(tv.data()),[q]"m"(square),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
        if(!small) {
            const u32 ten=k_10;
            asm volatile("flds 8(%[v])\n\tflds 4(%[v])\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(2)\n\t"
                "fmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfadds %[q]\n\tfsqrt\n\tfstp %%st(2)\n\tfstp %%st(0)\n\t"
                "flds %[t]\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[large]"
                :[large]"=qm"(large):[v]"r"(tv.data()),[q]"m"(square),[t]"m"(ten):H2_X87_CLOBBERS);
            if(!large)asm volatile("flds 8(%[v])\n\tflds 4(%[v])\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\tfld %%st(2)\n\t"
                "fmul %%st(3),%%st\n\tfaddp %%st,%%st(1)\n\tfadds %[q]\n\tfsqrt\n\tfstp %%st(2)\n\tfstp %%st(0)\n\tfstps %[r]"
                :[r]"=m"(rounded):[v]"r"(tv.data()),[q]"m"(square):H2_X87_CLOBBERS);
        }
        target_length=small?k_epsilon:(large?k_10:rounded);
    }
    // 3E67C..3E70F
    const auto ic=dvd(k_one,current_length),it=dvd(k_one,target_length);
    const auto tz=mul(tv[2],it),cx=mul(cv[0],ic),cy=mul(ic,cv[1]),cz=mul(ic,cv[2]),ty=mul(tv[1],it),tx=mul(tv[0],it);
    const auto dot=add(add(mul(tz,cz),mul(ty,cy)),mul(tx,cx));
    unsigned char aligned;const u32 limit=k_dot;
    asm volatile("flds %[d]\n\tfabs\n\tflds %[l]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[a]"
        :[a]"=qm"(aligned):[d]"m"(dot),[l]"m"(limit):H2_X87_CLOBBERS);
    if(aligned && gt(current_length,k_epsilon) && gt(target_length,k_epsilon)) {
        c.count(&NativeTu15ObjectLightingStats::rotations);
        // 3E732..3E7AA: rotation axis and the stepped angle (32CCD8 acos).
        u32 angle;
        asm volatile("flds %[d]\n\tfld1\n\tfadd %%st(1),%%st\n\tfld1\n\tfsub %%st(2),%%st\n\tfmulp %%st,%%st(1)\n\t"
            "fsqrt\n\tfxch %%st(1)\n\tfpatan\n\tfmuls %[s]\n\tfstps %[a]"
            :[a]"=m"(angle):[d]"m"(dot),[s]"m"(step):H2_X87_CLOBBERS);
        const auto ax=sub(mul(tz,cy),mul(ty,cz));
        const auto ay=sub(mul(cz,tx),mul(tz,cx));
        const auto az=sub(mul(ty,cx),mul(cy,tx));
        // 30BD0 on the axis local (its XMM0 result is unused).
        std::array<u32,3> axis{ax,ay,az};
        {
            u32 length;
            if(!length_small(axis,length)) {
                const auto inverse=dvd(k_one,length);
                axis={mul(axis[0],inverse),mul(inverse,axis[1]),mul(inverse,axis[2])};
            }
        }
        u32 sine,cosine;
        asm volatile("flds %[a]\n\tfsin\n\tfstps %[s]\n\tflds %[a]\n\tfcos\n\tfstps %[c]"
            :[s]"=m"(sine),[c]"=m"(cosine):[a]"m"(angle):H2_X87_CLOBBERS);
        const auto [nx,ny,nz]=axis;
        // 3E7B5..3E918 (XMM operand order as the original).
        const auto d=mul(add(add(mul(nz,cz),mul(ny,cy)),mul(nx,cx)),sub(k_one,cosine));
        const auto e1=sub(mul(cz,nx),mul(nz,cx));
        const auto e2=sub(mul(ny,cx),mul(cy,nx));
        const auto x=sub(add(mul(d,nx),mul(cosine,cx)),mul(sub(mul(nz,cy),mul(ny,cz)),sine));
        const auto y=sub(add(mul(ny,d),mul(cy,cosine)),mul(e1,sine));
        const auto z=sub(add(mul(nz,d),mul(cz,cosine)),mul(e2,sine));
        const auto length=add(clamp(sub(target_length,current_length),sub(k_zero,step),step),current_length);
        b.w32(current,mul(length,x));b.w32(current+4,mul(y,length));b.w32(current+8,mul(z,length));
        return;
    }
    // 3E91E..3E99F: step each component.
    const auto lower=sub(k_zero,step);
    auto delta=sub(b.r32(target),b.r32(current));
    if(gt(lower,delta))delta=lower;else if(gt(delta,step))delta=step;
    b.w32(current,add(delta,b.r32(current)));
    delta=sub(b.r32(target+4),b.r32(current+4));
    if(gt(lower,delta))delta=lower;else if(gt(delta,step))delta=step;
    b.w32(current+4,add(b.r32(current+4),delta));
    delta=clamp(sub(b.r32(target+8),b.r32(current+8)),lower,step);
    b.w32(current+8,add(b.r32(current+8),delta));
}
// 11A360(EAX out, ECX in, angle): limit the incident's elevation.
void limit_direction(Bank& b,u32 out,const std::array<u32,3>& in,u32 angle) {
    u32 cosine;unsigned char steep;
    asm volatile("flds %[a]\n\tfcos\n\tfstps %[c]\n\tflds 8(%[v])\n\tfabs\n\tflds %[c]\n\tfxch %%st(1)\n\t"
        "fcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[s]"
        :[c]"=m"(cosine),[s]"=qm"(steep):[a]"m"(angle),[v]"r"(in.data()):H2_X87_CLOBBERS);
    b.f3(out,in);
    if(steep)b.w32(out+8,gt(k_zero,in[2])?sub(k_zero,cosine):cosine);
    const u32 hundredth=k_hundredth;
    const auto small_component=[&](u32 at){
        const auto v=b.r32(at);unsigned char r;
        asm volatile("flds %[v]\n\tfabs\n\tflds %[k]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[r]"
            :[r]"=qm"(r):[v]"m"(v),[k]"m"(hundredth):H2_X87_CLOBBERS);return r!=0;};
    // Horizontal length: fld y; fld x; ...: (y*y + x*x).
    const auto horizontal=[&](u32& rounded){
        const auto x=b.r32(out),y=b.r32(out+4);unsigned char small;
        asm volatile("flds %[y]\n\tflds %[x]\n\tfld %%st(1)\n\tfmulp %%st,%%st(2)\n\tfld %%st(0)\n\tfmul %%st(1),%%st\n\t"
            "faddp %%st,%%st(2)\n\tfxch %%st(1)\n\tfsqrt\n\tfstp %%st(1)\n\t" H2_SMALL("%[r]")
            :[r]"=m"(rounded),[small]"=qm"(small):[x]"m"(x),[y]"m"(y),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
        return small!=0;};
    bool unit=false;
    if(small_component(out) && small_component(out+4))unit=true;
    else {
        u32 length;
        if(horizontal(length))length=k_zero;
        else {
            const auto inverse=dvd(k_one,length);
            b.w32(out,mul(b.r32(out),inverse));b.w32(out+4,mul(b.r32(out+4),inverse));
        }
        unsigned char small;
        asm volatile("flds %[v]\n\tfabs\n\tflds %[eps]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[s]"
            :[s]"=qm"(small):[v]"m"(length),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
        unit=small;
    }
    if(unit){b.w32(out,k_one);b.w32(out+4,k_zero);}
    {
        u32 length;
        if(!horizontal(length)) {
            const auto inverse=dvd(k_one,length);
            b.w32(out,mul(b.r32(out),inverse));b.w32(out+4,mul(b.r32(out+4),inverse));
        }
    }
    // 11A48F: scale the horizontal part by sqrt(1 - z*z). The +4 operand is
    // not an alias of the +0 store, so both are read first.
    const auto z=b.r32(out+8),x=b.r32(out),y=b.r32(out+4),one=k_one;u32 nx,ny;
    asm volatile("flds %[z]\n\tfld %%st(0)\n\tfmulp %%st,%%st(1)\n\tfsubrs %[one]\n\tfsqrt\n\tfld %%st(0)\n\t"
        "fmuls %[x]\n\tfstps %[nx]\n\tfmuls %[y]\n\tfstps %[ny]"
        :[nx]"=m"(nx),[ny]"=m"(ny):[z]"m"(z),[x]"m"(x),[y]"m"(y),[one]"m"(one):H2_X87_CLOBBERS);
    b.w32(out,nx);b.w32(out+4,ny);
}
// Rotate `at` about [474D10] by the angle ST0 (Rodrigues; D60D9/D6953).
struct Rotation {u32 sine,cosine;};
Rotation sine_cosine(u32 degrees) {
    Rotation r;const u32 scale=k_degrees;
    asm volatile("flds %[d]\n\tfmuls %[k]\n\tfld %%st(0)\n\tfsin\n\tfstps %[s]\n\tfcos\n\tfstps %[c]"
        :[s]"=m"(r.sine),[c]"=m"(r.cosine):[d]"m"(degrees),[k]"m"(scale):H2_X87_CLOBBERS);
    return r;
}
// 13DE70 through the native evaluator (refusals use the service; the
// evaluator and 13DE70 store nothing), then the D63..D65 output remap.
u32 authored(Context& c,u32 reference,u32 parameter) {
    u32 value;
    c.arithmetic.leave();
    try {value=native_tu15_evaluate_authored_function_extended(c.b.m,reference,parameter,k_one,c.fp,0);}
    catch(const std::invalid_argument&){c.arithmetic.resume();throw;}
    catch(const std::exception&) {
        c.count(&NativeTu15ObjectLightingStats::function_refusals);
        if(!c.services.function){c.arithmetic.resume();throw;}
        value=c.services.function(reference,parameter,k_one);
    }
    c.arithmetic.resume();
    const auto blob=c.b.r32(reference+4u);
    if(c.b.r8(blob+1u)&0xf0u)return value;
    const auto minimum=c.b.r32(blob+4u),maximum=c.b.r32(blob+8u);
    if(gt(k_zero,value))value=k_zero;else if(gt(value,k_one))value=k_one;
    return add(mul(sub(maximum,minimum),value),minimum);
}
// D5D30(EAX definition, ECX type, EDX sample, out, diffuse, lightmap).
bool convert_variable(Context& c,u32 definition,i32 type,const std::array<u32,5>& sample,u32 out,
    const std::array<u32,3>& diffuse,const std::array<u32,3>& lightmap) {
    auto& b=c.b;
    auto count=b.r32(definition);
    if(s32(count)<=0)return false;
    const auto mask=1u<<((u32(type)+1u)&31u);
    u32 variable=0;
    for(auto row=b.r32(definition+4u);count;--count,row+=0x90u) {
        const auto flags=b.r32(row);
        if((mask&flags) || (flags&1u))variable=row;
    }
    if(!variable)return false;
    const auto v=[&](u32 offset){return b.r32(variable+offset);};
    // D5D76..D5DCD: sample incident length; D5DD0..D5E3D normalized copy.
    std::array<u32,3> incident{sample[0],sample[1],sample[2]};u32 magnitude,stored,length;unsigned char small;
    asm volatile(H2_LEN3("%[v]") "fsts %[m]\n\tfstps %[o]":[m]"=m"(magnitude),[o]"=m"(stored)
        :[v]"r"(incident.data()):H2_X87_CLOBBERS);
    asm volatile("flds 8(%[v])\n\tfmuls 8(%[v])\n\tflds 4(%[v])\n\tfmuls 4(%[v])\n\tfaddp %%st,%%st(1)\n\t"
        "flds (%[v])\n\tfmuls (%[v])\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\t" H2_SMALL("%[r]")
        :[r]"=m"(length),[small]"=qm"(small):[v]"r"(incident.data()),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
    b.w32(out+0x18u,stored);
    if(!small) {
        const auto inverse=dvd(k_one,length);
        incident={mul(inverse,incident[0]),mul(incident[1],inverse),mul(incident[2],inverse)};
    }
    // D5E3D..D5EF9: offset lightmap and the primary color.
    const auto offset=v(4);
    const std::array<u32,3> light{add(lightmap[0],offset),add(lightmap[1],offset),add(lightmap[2],offset)};
    const auto pick=[&](u32 value,u32 low,u32 high){return gt(v(low),value)?v(low):(gt(value,v(high))?v(high):value);};
    b.w32(out+0x20u,pick(light[0],8,0x14));b.w32(out+0x24u,pick(light[1],0xc,0x18));b.w32(out+0x28u,pick(light[2],0x10,0x1c));
    // D5EFC..D5F44: primary direction, negated.
    limit_direction(b,out+0x2cu,incident,mul(v(0x20),k_degrees));
    for(u32 n=0;n<3;++n)b.w32(out+0x2cu+4u*n,sub(k_zero,b.r32(out+0x2cu+4u*n)));
    const auto squared=mul(magnitude,magnitude);
    // D5F49..D602E: secondary and diffuse clamps.
    const auto s0=pick(light[0],0x2c,0x38),s1=pick(light[1],0x30,0x3c),s2=pick(light[2],0x34,0x40);
    const auto d0=pick(diffuse[0],0x44,0x50),d1=pick(diffuse[1],0x48,0x54),d2=pick(diffuse[2],0x4c,0x58);
    // D602E..D60A4
    const auto remainder=sub(k_one,squared);
    b.w32(out+0x38u,add(mul(mul(mul(d0,s0),squared),k_two),mul(b.r32(out+0x20u),remainder)));
    b.w32(out+0x3cu,add(mul(b.r32(out+0x24u),remainder),mul(mul(mul(d1,s1),squared),k_two)));
    b.w32(out+0x40u,add(mul(b.r32(out+0x28u),remainder),mul(mul(mul(d2,s2),squared),k_two)));
    // D60A4..D61DA: secondary direction = -primary, rotated about [474D10].
    b.copy(out+0x44u,out+0x2cu,3);
    for(u32 n=0;n<3;++n)b.w32(out+0x44u+4u*n,sub(k_zero,b.r32(out+0x44u+4u*n)));
    {
        const auto up=b.f3(b.r32(0x474d10u));const auto s=b.f3(out+0x44u);
        const auto [sine,cosine]=sine_cosine(v(0x5c));
        const auto k=mul(add(add(mul(up[2],s[2]),mul(s[1],up[1])),mul(up[0],s[0])),sub(k_one,cosine));
        const auto e1=sub(mul(up[0],s[2]),mul(up[2],s[0]));
        const auto e2=sub(mul(up[1],s[0]),mul(s[1],up[0]));
        b.w32(out+0x44u,sub(add(mul(k,up[0]),mul(cosine,s[0])),mul(sub(mul(s[1],up[2]),mul(up[1],s[2])),sine)));
        b.w32(out+0x48u,sub(add(mul(up[1],k),mul(s[1],cosine)),mul(e1,sine)));
        b.w32(out+0x4cu,sub(add(mul(up[2],k),mul(cosine,s[2])),mul(e2,sine)));
    }
    // D61DF..D6243: ambient.
    b.w32(out,pick(light[0],0x68,0x74));b.w32(out+4u,pick(light[1],0x6c,0x78));b.w32(out+8u,pick(light[2],0x70,0x7c));
    // D6248..D631E: shadow direction = -incident, at most 75 degrees up.
    b.w32(out+0xcu,sub(k_zero,incident[0]));b.w32(out+0x10u,sub(k_zero,incident[1]));b.w32(out+0x14u,sub(k_zero,incident[2]));
    if(gt(b.r32(out+0x14u),k_m075)) {
        const auto z=b.r32(out+0x14u);
        b.w32(out+0x14u,gt(k_m1,z)?k_m1:(gt(z,k_m075)?k_m075:z));
        const auto shadow=b.f3(out+0xcu);u32 rounded;
        if(!length_small(shadow,rounded)) {
            const auto inverse=dvd(k_one,rounded);
            b.w32(out+0xcu,mul(b.r32(out+0xcu),inverse));
            b.w32(out+0x10u,mul(inverse,b.r32(out+0x10u)));
            b.w32(out+0x14u,mul(inverse,b.r32(out+0x14u)));
        }
    }
    // D631E..D6376: lightmap luminance.
    auto luminance=mul(add(add(mul(lightmap[2],k_011),mul(lightmap[1],k_059)),mul(lightmap[0],k_03)),k_two);
    if(gt(k_zero,luminance))luminance=k_zero;else if(gt(luminance,k_one))luminance=k_one;
    // D6376..D656D: authored scales.
    b.w32(out+0x1cu,authored(c,variable+0x88u,luminance));
    const auto primary=authored(c,variable+0x24u,magnitude);
    const auto secondary=authored(c,variable+0x60u,magnitude);
    b.w32(out+0x20u,mul(primary,b.r32(out+0x20u)));b.w32(out+0x24u,mul(b.r32(out+0x24u),primary));
    b.w32(out+0x28u,mul(b.r32(out+0x28u),primary));
    b.w32(out+0x38u,mul(secondary,b.r32(out+0x38u)));b.w32(out+0x3cu,mul(b.r32(out+0x3cu),secondary));
    b.w32(out+0x40u,mul(b.r32(out+0x40u),secondary));
    const auto dim=sub(k_one,add(mul(magnitude,k_01),k_06));
    const auto ambient=authored(c,variable+0x80u,add(add(mul(light[2],k_l114),mul(light[1],k_l587)),mul(light[0],k_l299)));
    const auto scale=mul(mul(ambient,dim),k_14);
    b.w32(out,mul(b.r32(out),scale));b.w32(out+4u,mul(b.r32(out+4u),scale));b.w32(out+8u,mul(b.r32(out+8u),scale));
    return true;
}
// D65C0(EAX sample, ECX type, ESI out): the 84-byte bank from a sample.
void convert(Context& c,const std::array<u32,5>& sample,i32 type,u32 out) {
    auto& b=c.b;c.count(&NativeTu15ObjectLightingStats::conversions);
    const auto lighting=b.r32(0x4ecc20u);
    u32 magnitude,length;unsigned char small;
    std::array<u32,3> incident{sample[0],sample[1],sample[2]};
    asm volatile(H2_LEN3("%[v]") "fstps %[m]":[m]"=m"(magnitude):[v]"r"(incident.data()):H2_X87_CLOBBERS);
    const bool character=type==0 || type==1 || type==2;
    // D661A..D669E: (x*x + z*z) + y*y.
    asm volatile("flds (%[v])\n\tfmuls (%[v])\n\tflds 8(%[v])\n\tfmuls 8(%[v])\n\tfaddp %%st,%%st(1)\n\t"
        "flds 4(%[v])\n\tfmuls 4(%[v])\n\tfaddp %%st,%%st(1)\n\tfsqrt\n\t" H2_SMALL("%[r]")
        :[r]"=m"(length),[small]"=qm"(small):[v]"r"(incident.data()),[eps]"m"(epsilon_bits):H2_X87_CLOBBERS);
    if(!small) {
        const auto inverse=dvd(k_one,length);
        incident={mul(inverse,incident[0]),mul(incident[1],inverse),mul(incident[2],inverse)};
    } else length=k_zero;
    b.w32(out+0x18u,length);
    const auto diffuse=unpack(sample[3]),lightmap=unpack(sample[4]);
    auto handle=b.r32(lighting+0x33cu);
    if(handle==none)handle=b.r32(b.r32(0x4ecc1cu)+0x184u);
    const auto definition=b.definition(handle);
    if(definition && convert_variable(c,definition,type,sample,out,diffuse,lightmap)) {}
    else {
        // D6704..D6BC7: the undefined-variable fallback.
        c.count(&NativeTu15ObjectLightingStats::conversion_fallbacks);
        b.f3(out+0x20u,lightmap);
        limit_direction(b,out+0x2cu,incident,b.r32(0x4739e4u));
        for(u32 n=0;n<3;++n)b.w32(out+0x2cu+4u*n,sub(k_zero,b.r32(out+0x2cu+4u*n)));
        std::array<u32,3> clamped;
        for(unsigned n=0;n<3;++n)
            asm volatile("flds %[v]\n\tfldl %[lo]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tjbe 1f\n\t"
                "fldl %[lo]\n\tjmp 3f\n1:\n\tfldl %[hi]\n\tflds %[v]\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\t"
                "jbe 2f\n\tfldl %[hi]\n\tjmp 3f\n2:\n\tflds %[v]\n3:\n\tfstps %[o]"
                :[o]"=m"(clamped[n]):[v]"m"(diffuse[n]),[lo]"m"(k_d650),[hi]"m"(k_d738):H2_X87_CLOBBERS);
        const auto squared=mul(magnitude,magnitude);
        const auto unit=[&](u32 value){return gt(k_zero,value)?k_zero:(gt(value,k_one)?k_one:value);};
        auto p0=unit(mul(mul(lightmap[0],clamped[0]),k_two));
        auto p1=unit(mul(mul(lightmap[1],clamped[1]),k_two));
        b.w32(out+0x38u,p0);
        b.w32(out+0x3cu,p1);
        auto p2=unit(mul(mul(lightmap[2],clamped[2]),k_two));
        b.w32(out+0x40u,p2);
        if(!gt(p0,k_zero))p0=k_zero;
        b.w32(out+0x38u,p0);
        if(!gt(p1,k_zero))p1=k_zero;
        b.w32(out+0x3cu,p1);
        if(!gt(p2,k_zero))p2=k_zero;
        const auto remainder=sub(k_one,squared);
        b.w32(out+0x38u,add(mul(remainder,b.r32(out+0x20u)),mul(p0,squared)));
        b.w32(out+0x3cu,add(mul(b.r32(out+0x24u),remainder),mul(p1,squared)));
        b.w32(out+0x40u,add(mul(b.r32(out+0x28u),remainder),mul(p2,squared)));
        b.copy(out+0x44u,out+0x2cu,3);
        for(u32 n=0;n<3;++n)b.w32(out+0x44u+4u*n,sub(k_zero,b.r32(out+0x44u+4u*n)));
        {
            const auto up=b.f3(b.r32(0x474d10u));const auto s=b.f3(out+0x44u);
            const auto [sine,cosine]=sine_cosine(b.r32(0x4739f4u));
            const auto k=mul(add(add(mul(up[2],s[2]),mul(up[0],s[0])),mul(s[1],up[1])),sub(k_one,cosine));
            const auto e1=sub(mul(up[0],s[2]),mul(up[2],s[0]));
            const auto e2=sub(mul(s[0],up[1]),mul(up[0],s[1]));
            b.w32(out+0x44u,sub(add(mul(up[0],k),mul(s[0],cosine)),mul(sub(mul(up[2],s[1]),mul(s[2],up[1])),sine)));
            b.w32(out+0x48u,sub(add(mul(k,up[1]),mul(cosine,s[1])),mul(e1,sine)));
            b.w32(out+0x4cu,sub(add(mul(up[2],k),mul(s[2],cosine)),mul(e2,sine)));
        }
        b.f3(out,lightmap);
        b.w32(out+0xcu,sub(k_zero,incident[0]));b.w32(out+0x10u,sub(k_zero,incident[1]));b.w32(out+0x14u,sub(k_zero,incident[2]));
        b.w32(out+0x14u,mul(b.r32(out+0x14u),k_two));
        (void)normalize(b,out+0xcu);
        const auto luminance=add(add(mul(lightmap[2],k_l114),mul(lightmap[1],k_l587)),mul(lightmap[0],k_l299));
        const auto contrast=character?k_118:k_075;
        const auto dim=sub(k_one,add(mul(magnitude,k_01),k_06));
        const auto ambient=mul(dim,add(mul(sub(contrast,k_07),luminance),k_07));
        b.w32(out,mul(b.r32(out),ambient));b.w32(out+4u,mul(b.r32(out+4u),ambient));b.w32(out+8u,mul(b.r32(out+8u),ambient));
        const auto edge=mul(magnitude,k_005);
        const auto primary=add(edge,k_04);
        b.w32(out+0x20u,mul(primary,b.r32(out+0x20u)));b.w32(out+0x24u,mul(b.r32(out+0x24u),primary));
        b.w32(out+0x28u,mul(b.r32(out+0x28u),primary));
        const auto secondary=sub(k_04,edge);
        b.w32(out+0x38u,mul(b.r32(out+0x38u),secondary));b.w32(out+0x3cu,mul(b.r32(out+0x3cu),secondary));
        b.w32(out+0x40u,mul(b.r32(out+0x40u),secondary));
        b.w32(out+0x1cu,k_one);
    }
    // D6BD1..D6D0B: clamp the colors to [0,1].
    for(const auto at:{0x20u,0x24u,0x28u,0x38u,0x3cu,0x40u,0u,4u,8u}) {
        const auto value=b.r32(out+at);
        b.w32(out+at,gt(k_zero,value)?k_zero:(gt(value,k_one)?k_one:value));
    }
}
// BC200(EAX object, ECX linear): the root's linear velocity.
std::array<u32,3> velocity(const Bank& b,u32 object) {
    const auto body=b.body(root(b,object)),physics=b.r32(body+0xb4u);
    if(physics!=none) {
        const auto row=b.r32(b.r32(0x52be5cu)+0x44u)+(physics&65535u)*0xa0u;
        const auto selected=std::bit_cast<std::int8_t>(std::uint8_t(b.r8(row+0x18u)));
        const i32 index=(selected<0 || i32(selected)>=s32(b.r32(row+0x74u)))?-1:selected;
        if(index!=-1) {
            if(b.r8(body+0xc1u)&1u)return b.f3(b.r32(0x474d04u));
            // 1D8C80
            const auto rigid=b.r32(u32(index)*96u+b.r32(row+0x70u)+0x40u);
            if(b.r8(rigid+0x40u))return b.f3(b.r32(0x474d04u));
            const auto havok=b.r32(rigid+0x3cu);
            return {b.r32(havok+0x40u),b.r32(havok+0x44u),b.r32(havok+0x48u)};
        }
    }
    return b.f3(body+0x88u);
}
}  // namespace

void native_tu15_object_lighting_update_3dfd0(SourcePhysicsContactMemory& memory,std::uint32_t token,
    std::uint32_t object,std::uint32_t metric,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15ObjectLightingServices& services,NativeTu15ObjectLightingStats* stats) {
    Arithmetic arithmetic(fp);Context c{{memory},fp,arithmetic,services,stats};auto& b=c.b;
    c.count(&NativeTu15ObjectLightingStats::updates);
    const auto frame=b.r32(0x4c6854u),record=b.record(token),stamp=b.r32(record+8u);
    auto delta=s32(frame-stamp);
    if(!delta){c.count(&NativeTu15ObjectLightingStats::unchanged);return;}
    const auto body=b.body(object),last=b.r32(record+12u);
    const auto since=s32(frame-last);
    const bool fresh=!b.r8(record+3u) || !b.r8(record+2u);
    if(delta<0)delta=1;
    bool snap=false;
    if(fresh)snap=true;
    else if(b.r8(body+0xc1u)&1u){c.count(&NativeTu15ObjectLightingStats::frozen);return;}
    // 3E065..3E0B5: the query cadence by lighting metric.
    bool due=true;
    if(stamp!=none) {
        const i32 limit=gt(metric,k_400)?3:gt(metric,k_200)?6:gt(metric,k_100)?9:gt(metric,k_50)?15:gt(metric,k_10)?45:60;
        due=delta>limit;
    }
    if(last==none || since>1)snap=true;
    if(due) {
        c.count(&NativeTu15ObjectLightingStats::due);
        bool query=true;
        if(b.r8(record+2u)) {
            const auto dz=sub(b.r32(body+0x38u),b.r32(record+0x18u)),dx=sub(b.r32(body+0x30u),b.r32(record+0x10u)),
                dy=sub(b.r32(body+0x34u),b.r32(record+0x14u));
            if(gt(k_near,add(add(mul(dz,dz),mul(dx,dx)),mul(dy,dy)))){query=false;c.count(&NativeTu15ObjectLightingStats::near_skips);}
        }
        if(query) {
            c.count(&NativeTu15ObjectLightingStats::queries);
            b.w32(record+8u,b.r32(0x4c6854u));
            if(!services.query)throw std::runtime_error("Native TU1.5 object lighting has no D3D90 query owner");
            arithmetic.leave();bool hit;
            try{hit=services.query(object,fresh?1u:0u,record+0x30u);}catch(...){arithmetic.resume();throw;}
            arithmetic.resume();
            if(hit){c.count(&NativeTu15ObjectLightingStats::query_hits);b.w8(record+2u,1);b.copy(record+0x10u,body+0x30u,3);}
        }
    }
    if(!b.r8(record+2u))return;
    if(b.r8(record+3u) && b.r8(0x4739c4u) && !snap) {
        c.count(&NativeTu15ObjectLightingStats::interpolations);
        // 3E18E..3E278: speed-dependent interpolation toward the target.
        const auto linear=velocity(b,object);u32 speed;unsigned char negative;
        const u32 base=k_speed,scale=k_speed_scale,zero=k_zero;
        asm volatile(H2_LEN3("%[v]") "fsubs %[b]\n\tfmuls %[s]\n\tfsts %[t]\n\tflds %[z]\n\tfcomip %%st(1),%%st\n\t"
            "fstp %%st(0)\n\tseta %[n]"
            :[t]"=m"(speed),[n]"=qm"(negative):[v]"r"(linear.data()),[b]"m"(base),[s]"m"(scale),[z]"m"(zero):H2_X87_CLOBBERS);
        u32 amount;bool step=true;
        if(negative){speed=k_zero;amount=k_zero;}
        if(!negative && gt(speed,k_one))amount=k_one;
        else if(!negative && gt(speed,k_zero))amount=speed;
        else {
            unsigned char moved;const u32 threshold=k_moved;
            const auto origin=b.f3(body+0x30u),query=b.f3(record+0x10u);
            asm volatile("flds (%[a])\n\tfsubs (%[q])\n\tflds 4(%[a])\n\tfsubs 4(%[q])\n\tflds 8(%[a])\n\tfsubs 8(%[q])\n\t"
                "fld %%st(2)\n\tfmulp %%st,%%st(3)\n\tfld %%st(1)\n\tfmul %%st(2),%%st\n\tfaddp %%st,%%st(3)\n\t"
                "fld %%st(0)\n\tfmul %%st(1),%%st\n\tfaddp %%st,%%st(3)\n\tfxch %%st(2)\n\tfsqrt\n\tfstp %%st(2)\n\t"
                "fstp %%st(0)\n\tflds %[k]\n\tfxch %%st(1)\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %[m]"
                :[m]"=qm"(moved):[a]"r"(origin.data()),[q]"r"(query.data()),[k]"m"(threshold):H2_X87_CLOBBERS);
            if(moved && gt(speed,k_zero))amount=speed;
            else if(typed_body(b,object,0x80u))amount=speed;
            else step=false;
        }
        if(step) {
            const auto color=mul(amount,k_half),incident=mul(amount,k_hundredth);
            step_incident(c,record+0x1cu,record+0x30u,incident);
            step_color(b,record+0x2cu,record+0x40u,color);
            step_color(b,record+0x28u,record+0x3cu,color);
        }
    } else {
        c.count(&NativeTu15ObjectLightingStats::snaps);
        b.copy(record+0x1cu,record+0x30u,5);
    }
    // 3E29E..3E2FE
    const auto owner=b.r32(record+4u);i32 type=-1;
    if(owner!=none) {
        const auto row=b.body(owner);
        type=sx16(b.r16(b.definition(b.r32(row))));
    }
    std::array<u32,5> sample;for(u32 n=0;n<5;++n)sample[n]=b.r32(record+0x1cu+4u*n);
    convert(c,sample,type,record+0x54u);
    b.w8(record+3u,1);
}

std::uint32_t native_tu15_object_lighting_3daf0(SourcePhysicsContactMemory& memory,std::uint32_t object,
    std::uint32_t metric,NativeTu15PhysicsFrameFp& fp,const NativeTu15ObjectLightingServices& services,
    NativeTu15ObjectLightingStats* stats) {
    if(fp.control_word!=0x027fu || (fp.status&0x8080u) || (fp.mxcsr&0x1f80u)!=0x1f80u || (fp.mxcsr&0xffff0000u))
        throw std::invalid_argument("Native TU1.5 object lighting requires the original 027F control word and masked exceptions");
    Bank b{memory};
    if(stats)++stats->calls;
    const auto body=b.body(object);
    if(b.r8(body+7u)&1u){if(stats)++stats->default_banks;return b.r32(0x4eabc4u)+0x1cu;}
    const auto probe=b.r16(body+0xd0u);
    if(probe!=0xffffu){if(stats)++stats->probe_banks;return u32(sx16(probe))*0x5cu+b.r32(b.r32(0x4ecc18u)+0x230u)+8u;}
    const auto owner=root(b,object);
    // A missing root record stores nothing first; see the header.
    const auto token=native_tu15_object_lighting_acquire_3ddc0(memory,owner,fp,stats);
    if(token==none)throw NativeTu15ObjectLightingDecline("Native TU1.5 object lighting root has no render-cache record");
    const auto record=b.record(token);
    if(owner==object) {
        if(stats)++stats->roots;
        native_tu15_object_lighting_update_3dfd0(memory,token,object,metric,fp,services,stats);
        return record+0x54u;
    }
    if(stats)++stats->children;
    const auto child=native_tu15_object_lighting_acquire_3ddc0(memory,object,fp,stats);
    if(child==none){if(stats)++stats->uncached_children;return record+0x54u;}
    const auto copy=b.r32(b.r32(0x515f14u)+0x44u)+((child&65535u)<<8);
    b.copy(copy+0x54u,record+0x54u,21);
    b.copy(copy+0x30u,record+0x1cu,5);
    b.copy(copy+0x1cu,record+0x1cu,5);
    return record+0x54u;
}
#else
std::uint32_t native_tu15_object_lighting_3daf0(SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t,
    NativeTu15PhysicsFrameFp&,const NativeTu15ObjectLightingServices&,NativeTu15ObjectLightingStats*) {
    throw std::runtime_error("Native TU1.5 object lighting needs the x86 arithmetic backend");
}
std::uint32_t native_tu15_object_lighting_acquire_3ddc0(SourcePhysicsContactMemory&,std::uint32_t,
    NativeTu15PhysicsFrameFp&,NativeTu15ObjectLightingStats*) {
    throw std::runtime_error("Native TU1.5 object lighting needs the x86 arithmetic backend");
}
void native_tu15_object_lighting_update_3dfd0(SourcePhysicsContactMemory&,std::uint32_t,std::uint32_t,std::uint32_t,
    NativeTu15PhysicsFrameFp&,const NativeTu15ObjectLightingServices&,NativeTu15ObjectLightingStats*) {
    throw std::runtime_error("Native TU1.5 object lighting needs the x86 arithmetic backend");
}
#endif
}
