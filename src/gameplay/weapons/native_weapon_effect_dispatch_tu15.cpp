#include "native_tu15_effect_transform_product.hpp"
#include "native_tu15_effect_datum_allocate.hpp"
#include "native_tu15_effect_slot_scan.hpp"
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include <bit>
#include <cfloat>
#include <cstring>
#include <stdexcept>
#include <immintrin.h>

namespace halo2 {
namespace {
using Memory=SourcePhysicsContactMemory;
using Abi=SourcePhysicsWorldRuntimeAbi;
using Observer=SourcePhysicsContactRecordObserver;
using Services=SourcePhysicsWorldRuntimeServices;
constexpr auto none=0xffffffffu;
struct IteratorRegisters {std::array<std::uint32_t,8> &gp;std::uint32_t &eflags;};
std::uint32_t iterator_flags(IteratorRegisters &,std::uint32_t,std::uint32_t,unsigned,bool);
void iterator_logic_flags(IteratorRegisters &,std::uint32_t,unsigned);
std::uint32_t read(const Memory &m,std::uint32_t at,unsigned width=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(width));
    std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
    return value;
}
template<class Registers>void push(Memory &m,Registers &a,const Observer &observe,std::uint32_t pc,std::uint32_t value) {
    // A faulting PUSH/CALL does not commit ESP on the original CPU.
    const auto next_stack=a.gp[4]-4u;
    SourcePhysicsContactRecordStore store{pc,next_stack,{},std::vector<std::byte>(4)};
    if(observe){store.before.resize(4);m.read(store.address,store.before);}
    for(unsigned n=0;n<4;++n)store.written[n]=std::byte(value>>(8*n));
    m.write(store.address,store.written);a.gp[4]=next_stack;if(observe)observe(store);
}
void store(Memory &m,const Observer &observe,std::uint32_t pc,std::uint32_t at,
    std::uint32_t value,unsigned width=4) {
    SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>(width)};
    if(observe){event.before.resize(width);m.read(at,event.before);}
    for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8*n));
    m.write(at,event.written);if(observe)observe(event);
}
template<class Registers>std::uint32_t pop(const Memory &m,Registers &a) {
    const auto value=read(m,a.gp[4]);a.gp[4]+=4;return value;
}
void call(Memory &m,Abi &a,const Observer &o,const Services &s,std::uint32_t pc,std::uint32_t target) {
    if(s.instruction)s.instruction(pc);
    push(m,a,o,pc,pc+5);
    if(!s.required_owner)throw std::runtime_error("Native weapon effect requires its actual callee owner");
    s.required_owner(m,{pc,target,false},a,o);
}
__m128 vector(const Abi &a,unsigned n) {
    return _mm_castsi128_ps(_mm_loadu_si128(reinterpret_cast<const __m128i*>(a.xmm[n].data())));
}
void vector(Abi &a,unsigned n,__m128 value) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(a.xmm[n].data()),_mm_castps_si128(value));
}
void scalar(const Memory &m,Abi &a,unsigned n,std::uint32_t at) {
    a.xmm[n]={read(m,at),0,0,0};
}
// This leaf only uses SSE. Preserve the caller's x87 file and host FP state,
// while returning the original SSE sticky flags even on a stopped memory read.
struct SseEnvironment {
    Abi &a;unsigned host;
    explicit SseEnvironment(Abi &abi):a(abi),host(_mm_getcsr()) {
        if(!a.floating_point)throw std::runtime_error("Native effect origin requires caller FP controls");
        if((a.floating_point->mxcsr&0x1f80u)!=0x1f80u)
            throw std::runtime_error("Native effect origin requires masked SSE exceptions");
        _mm_setcsr(a.floating_point->mxcsr);
    }
    ~SseEnvironment(){a.floating_point->mxcsr=_mm_getcsr();_mm_setcsr(host);}
};
// BBE00: unattached objects copy their position. Attached objects transform
// that position through their signed parent-node matrix, in retail SSE order.
void effect_origin(Memory &m,Abi &a,const Observer &o,const Services &s) {
    bool sse_active{};
    const auto instruction=[&](auto pc){if(sse_active)a.floating_point->mxcsr=_mm_getcsr();if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0xbbe00);a.gp[1]=read(m,0x4ecbd0);
    instruction(0xbbe06);a.gp[2]=read(m,a.gp[1]+0x44);
    instruction(0xbbe09);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0xbbe0e);a.gp[0]*=3u;
    instruction(0xbbe11);a.gp[0]=read(m,a.gp[2]+a.gp[0]*4u+8);
    instruction(0xbbe15);a.gp[1]=read(m,a.gp[0]+0x14);
    instruction(0xbbe18);iterator_flags(r,a.gp[1],none,4,true);
    instruction(0xbbe1b);
    if(a.eflags&64u) {
        instruction(0xbbe1d);a.gp[0]=iterator_flags(r,a.gp[0],0x64,4,false);
        instruction(0xbbe20);a.gp[2]=read(m,a.gp[0]);
        instruction(0xbbe22);a.gp[1]=a.gp[6];
        instruction(0xbbe24);store(m,o,0xbbe24,a.gp[1],a.gp[2]);
        instruction(0xbbe26);a.gp[2]=read(m,a.gp[0]+4);
        instruction(0xbbe29);store(m,o,0xbbe29,a.gp[1]+4,a.gp[2]);
        instruction(0xbbe2c);a.gp[0]=read(m,a.gp[0]+8);
        instruction(0xbbe2f);store(m,o,0xbbe2f,a.gp[1]+8,a.gp[0]);
        instruction(0xbbe32);a.gp[0]=a.gp[6];
        instruction(0xbbe34);(void)pop(m,a);return;
    }
    instruction(0xbbe35);scalar(m,a,0,a.gp[0]+0x64);
    instruction(0xbbe3a);scalar(m,a,1,a.gp[0]+0x68);
    instruction(0xbbe3f);scalar(m,a,2,a.gp[0]+0x6c);
    instruction(0xbbe44);push(m,a,o,0xbbe44,a.gp[7]);
    instruction(0xbbe45);a.gp[7]=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(read(m,a.gp[0]+0x18,1)))));
    instruction(0xbbe49);a.gp[7]*=0x34u;a.eflags&=~0x801u;
    instruction(0xbbe4c);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0xbbe52);a.gp[1]*=3u;
    instruction(0xbbe55);a.gp[2]=read(m,a.gp[2]+a.gp[1]*4u+8);
    instruction(0xbbe59);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[2]+0x116,2)))));
    instruction(0xbbe60);a.gp[7]=iterator_flags(r,a.gp[7],a.gp[2],4,false);
    instruction(0xbbe62);scalar(m,a,3,a.gp[1]+a.gp[7]);
    instruction(0xbbe67);a.gp[1]=iterator_flags(r,a.gp[1],a.gp[7],4,false);
    SseEnvironment environment(a);sse_active=true;
    instruction(0xbbe69);
    const auto one=_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,0x46a0d0)));const auto scale=vector(a,3);auto ax=std::uint16_t(a.gp[0]);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("ucomiss %2,%1; lahf":"+a"(ax):"x"(scale),"x"(one):"cc");
#else
    throw std::runtime_error("Native effect origin scalar comparison backend unavailable");
#endif
    a.eflags=(a.eflags&~0x8d5u)|((ax>>8u)&0xd5u);
    instruction(0xbbe70);a.gp[0]=(a.gp[0]&0xffff00ffu)|(ax&0xff00u);
    instruction(0xbbe71);iterator_logic_flags(r,(a.gp[0]>>8u)&0x44u,1);
    instruction(0xbbe74);a.gp[7]=pop(m,a);
    instruction(0xbbe75);if(a.eflags&4u) {
        instruction(0xbbe77);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,0)));
        instruction(0xbbe7b);a.xmm[0]=a.xmm[3];
        instruction(0xbbe7e);scalar(m,a,3,a.gp[1]);
        instruction(0xbbe82);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,1)));
        instruction(0xbbe86);a.xmm[1]=a.xmm[3];
        instruction(0xbbe89);scalar(m,a,3,a.gp[1]);
        instruction(0xbbe8d);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,2)));
        instruction(0xbbe91);a.xmm[2]=a.xmm[3];
    }
    // Loads remain after each preceding store when output aliases the matrix.
    instruction(0xbbe94);scalar(m,a,3,a.gp[1]+0x1c);
    instruction(0xbbe99);scalar(m,a,4,a.gp[1]+0x10);
    instruction(0xbbe9e);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,2)));
    instruction(0xbbea2);vector(a,4,_mm_mul_ss(vector(a,4),vector(a,1)));
    instruction(0xbbea6);vector(a,3,_mm_add_ss(vector(a,3),vector(a,4)));
    instruction(0xbbeaa);scalar(m,a,4,a.gp[1]+4);
    instruction(0xbbeaf);vector(a,4,_mm_mul_ss(vector(a,4),vector(a,0)));
    instruction(0xbbeb3);vector(a,3,_mm_add_ss(vector(a,3),vector(a,4)));
    instruction(0xbbeb7);vector(a,3,_mm_add_ss(vector(a,3),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+0x28)))));
    instruction(0xbbebc);store(m,o,0xbbebc,a.gp[6],a.xmm[3][0]);
    instruction(0xbbec0);scalar(m,a,3,a.gp[1]+0x20);
    instruction(0xbbec5);scalar(m,a,4,a.gp[1]+0x14);
    instruction(0xbbeca);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,2)));
    instruction(0xbbece);vector(a,4,_mm_mul_ss(vector(a,4),vector(a,1)));
    instruction(0xbbed2);vector(a,3,_mm_add_ss(vector(a,3),vector(a,4)));
    instruction(0xbbed6);scalar(m,a,4,a.gp[1]+8);
    instruction(0xbbedb);vector(a,4,_mm_mul_ss(vector(a,4),vector(a,0)));
    instruction(0xbbedf);vector(a,3,_mm_add_ss(vector(a,3),vector(a,4)));
    instruction(0xbbee3);vector(a,3,_mm_add_ss(vector(a,3),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+0x2c)))));
    instruction(0xbbee8);store(m,o,0xbbee8,a.gp[6]+4,a.xmm[3][0]);
    instruction(0xbbeed);scalar(m,a,3,a.gp[1]+0x24);
    instruction(0xbbef2);vector(a,3,_mm_mul_ss(vector(a,3),vector(a,2)));
    instruction(0xbbef6);scalar(m,a,2,a.gp[1]+0x18);
    instruction(0xbbefb);vector(a,2,_mm_mul_ss(vector(a,2),vector(a,1)));
    instruction(0xbbeff);scalar(m,a,1,a.gp[1]+12);
    instruction(0xbbf04);vector(a,3,_mm_add_ss(vector(a,3),vector(a,2)));
    instruction(0xbbf08);vector(a,1,_mm_mul_ss(vector(a,1),vector(a,0)));
    instruction(0xbbf0c);vector(a,3,_mm_add_ss(vector(a,3),vector(a,1)));
    instruction(0xbbf10);vector(a,3,_mm_add_ss(vector(a,3),_mm_castsi128_ps(_mm_cvtsi32_si128(read(m,a.gp[1]+0x30)))));
    instruction(0xbbf15);store(m,o,0xbbf15,a.gp[6]+8,a.xmm[3][0]);
    instruction(0xbbf1a);a.gp[0]=a.gp[6];
    instruction(0xbbf1c);(void)pop(m,a);
}
// 1791C0: two exact direction records. Reads stay interleaved with writes,
// including when the input, output and global vectors overlap.
void direction_packets(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    instruction(0x1791c0);push(m,a,o,0x1791c0,a.gp[6]);
    instruction(0x1791c1);a.gp[6]=read(m,0x474d1c);
    instruction(0x1791c7);push(m,a,o,0x1791c7,a.gp[7]);
    instruction(0x1791c8);a.gp[7]=read(m,a.gp[6]);
    instruction(0x1791ca);a.gp[2]=a.gp[0]+12;
    instruction(0x1791cd);store(m,o,0x1791cd,a.gp[2],a.gp[7]);
    instruction(0x1791cf);a.gp[7]=read(m,a.gp[6]+4);
    instruction(0x1791d2);store(m,o,0x1791d2,a.gp[2]+4,a.gp[7]);
    instruction(0x1791d5);a.gp[6]=read(m,a.gp[6]+8);
    instruction(0x1791d8);store(m,o,0x1791d8,a.gp[2]+8,a.gp[6]);
    instruction(0x1791db);a.gp[2]=a.gp[1];
    instruction(0x1791dd);a.gp[7]=read(m,a.gp[2]);
    instruction(0x1791df);a.gp[6]=a.gp[0];
    instruction(0x1791e1);store(m,o,0x1791e1,a.gp[6],a.gp[7]);
    instruction(0x1791e3);a.gp[7]=read(m,a.gp[2]+4);
    instruction(0x1791e6);store(m,o,0x1791e6,a.gp[6]+4,a.gp[7]);
    instruction(0x1791e9);a.gp[2]=read(m,a.gp[2]+8);
    instruction(0x1791ec);store(m,o,0x1791ec,a.gp[6]+8,a.gp[2]);
    instruction(0x1791ef);a.gp[6]=read(m,0x474d10);
    instruction(0x1791f5);store(m,o,0x1791f5,a.gp[0]+0x18,0x070000c0);
    instruction(0x1791fc);a.gp[7]=read(m,a.gp[6]);
    instruction(0x1791fe);a.gp[2]=a.gp[0]+0x28;
    instruction(0x179201);store(m,o,0x179201,a.gp[2],a.gp[7]);
    instruction(0x179203);a.gp[7]=read(m,a.gp[6]+4);
    instruction(0x179206);store(m,o,0x179206,a.gp[2]+4,a.gp[7]);
    instruction(0x179209);a.gp[6]=read(m,a.gp[6]+8);
    instruction(0x17920c);store(m,o,0x17920c,a.gp[2]+8,a.gp[6]);
    instruction(0x17920f);a.gp[6]=read(m,a.gp[1]);
    instruction(0x179211);a.gp[2]=a.gp[0]+0x1c;
    instruction(0x179214);store(m,o,0x179214,a.gp[2],a.gp[6]);
    instruction(0x179216);a.gp[6]=read(m,a.gp[1]+4);
    instruction(0x179219);store(m,o,0x179219,a.gp[2]+4,a.gp[6]);
    instruction(0x17921c);a.gp[1]=read(m,a.gp[1]+8);
    instruction(0x17921f);a.gp[7]=pop(m,a);
    instruction(0x179220);store(m,o,0x179220,a.gp[2]+8,a.gp[1]);
    instruction(0x179223);store(m,o,0x179223,a.gp[0]+0x34,0x020000ca);
    instruction(0x17922a);a.gp[6]=pop(m,a);
    instruction(0x17922b);(void)pop(m,a);
}
void select_object(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x106f90);a.gp[2]=read(m,0x4ecbd0);
    instruction(0x106f96);a.gp[2]=read(m,a.gp[2]+0x44);
    instruction(0x106f99);a.gp[1]=a.gp[0];
    instruction(0x106f9b);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x106fa1);a.gp[1]*=3u;
    instruction(0x106fa4);push(m,a,o,0x106fa4,a.gp[3]);
    instruction(0x106fa5);a.gp[3]=(a.gp[3]&0xffffff00u)|read(m,a.gp[2]+a.gp[1]*4u+2u,1);
    instruction(0x106fa9);push(m,a,o,0x106fa9,a.gp[6]);
    instruction(0x106faa);a.gp[1]=a.gp[2]+a.gp[1]*4u;
    instruction(0x106fad);a.gp[2]=read(m,a.gp[1]+8u);
    instruction(0x106fb0);a.gp[6]=read(m,a.gp[2]);
    instruction(0x106fb2);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
    instruction(0x106fb8);push(m,a,o,0x106fb8,a.gp[7]);
    instruction(0x106fb9);a.gp[7]=read(m,0x4f0414);
    instruction(0x106fbf);a.gp[6]<<=4u;iterator_logic_flags(r,a.gp[6],4);a.eflags&=~0x801u;
    instruction(0x106fc2);iterator_logic_flags(r,a.gp[3]&0x10u,1);
    // The tag lookup is eager even when a hidden header selects the parent.
    instruction(0x106fc5);a.gp[6]=read(m,a.gp[6]+a.gp[7]+8);
    instruction(0x106fc9);if(!(a.eflags&64u))goto parent;
    instruction(0x106fcb);iterator_logic_flags(r,read(m,a.gp[2]+4,1)&1u,1);
    instruction(0x106fcf);if(!(a.eflags&64u))goto parent;
    instruction(0x106fd1);iterator_flags(r,read(m,a.gp[6]+0x38),none,4,true);
    instruction(0x106fd5);if(!(a.eflags&64u))goto complete;
parent:
    instruction(0x106fd7);a.gp[2]=read(m,a.gp[2]+0x14);
    instruction(0x106fda);iterator_flags(r,a.gp[2],none,4,true);
    instruction(0x106fdd);if(a.eflags&64u)goto complete;
    instruction(0x106fdf);a.gp[0]=a.gp[2];
complete:
    instruction(0x106fe1);a.gp[7]=pop(m,a);
    instruction(0x106fe2);a.gp[6]=pop(m,a);
    instruction(0x106fe3);a.gp[3]=pop(m,a);
    instruction(0x106fe4);(void)pop(m,a);
}
void dispatch(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x108a70);push(m,a,o,0x108a70,a.gp[3]);
    instruction(0x108a71);push(m,a,o,0x108a71,a.gp[6]);
    instruction(0x108a72);push(m,a,o,0x108a72,a.gp[7]);
    instruction(0x108a73);a.gp[3]=a.gp[0];
    instruction(0x108a75);a.gp[7]=none;iterator_logic_flags(r,a.gp[7],4);
    instruction(0x108a78);iterator_flags(r,a.gp[3],none,4,true);
    instruction(0x108a7b);a.gp[6]=a.gp[1];
    instruction(0x108a7d);if(a.eflags&64u)goto no_effect;
    instruction(0x108a83);push(m,a,o,0x108a83,a.gp[5]);
    instruction(0x108a84);a.gp[0]=a.gp[6];
    instruction(0x108a86);push(m,a,o,0x108a86,0x108a8b);select_object(m,a,o,s);
    instruction(0x108a8b);a.gp[1]=read(m,0x4ecbd0);
    instruction(0x108a91);a.gp[2]=read(m,a.gp[1]+0x44);
    instruction(0x108a94);a.gp[6]&=65535u;iterator_logic_flags(r,a.gp[6],4);
    instruction(0x108a9a);a.gp[5]=a.gp[0];
    instruction(0x108a9c);a.gp[0]=a.gp[6]*3u;
    instruction(0x108a9f);a.gp[0]=read(m,a.gp[2]+a.gp[0]*4u+8);
    instruction(0x108aa3);a.gp[2]=(a.gp[2]&0xffffff00u)|read(m,a.gp[0]+0x12c,1);
    instruction(0x108aa9);a.gp[1]=none;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x108aac);iterator_logic_flags(r,a.gp[2]&1u,1);
    instruction(0x108aaf);
    if(!(a.eflags&64u)){instruction(0x108ab1);a.gp[1]=read(m,a.gp[0]+0x154);}
    instruction(0x108ab7);a.gp[2]=read(m,0x4f0414);
    instruction(0x108abd);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[3]))));
    instruction(0x108ac0);{
        const auto before=a.gp[0];a.gp[0]<<=4;iterator_logic_flags(r,a.gp[0],4);
        a.eflags=(a.eflags&~0x801u)|((before>>28u)&1u);
    }
    instruction(0x108ac3);a.gp[0]=read(m,a.gp[2]+a.gp[0]);
    instruction(0x108ac6);iterator_flags(r,a.gp[0],0x65666665u,4,true);
    instruction(0x108acb);if(a.eflags&64u)goto effect;
    instruction(0x108acd);iterator_flags(r,a.gp[0],0x736e6421u,4,true);
    instruction(0x108ad2);if(!(a.eflags&64u))goto restored_effect;
    instruction(0x108ad4);a.gp[0]=read(m,a.gp[4]+0x18);
    instruction(0x108ad8);push(m,a,o,0x108ad8,a.gp[0]);
    instruction(0x108ad9);a.gp[0]=read(m,0x474d08);
    instruction(0x108ade);push(m,a,o,0x108ade,none);
    instruction(0x108ae0);push(m,a,o,0x108ae0,a.gp[1]);
    instruction(0x108ae1);a.gp[1]=read(m,0x474ce8);
    call(m,a,o,s,0x108ae7,0x18be50);
restored_effect:
    instruction(0x108aec);a.gp[5]=pop(m,a);
    instruction(0x108aed);a.gp[0]=a.gp[7];
    instruction(0x108aef);a.gp[7]=pop(m,a);
    instruction(0x108af0);a.gp[6]=pop(m,a);
    instruction(0x108af1);a.gp[3]=pop(m,a);
    instruction(0x108af2);(void)pop(m,a);a.gp[4]+=12;return;
effect:
    instruction(0x108af5);a.gp[7]=read(m,a.gp[4]+0x14);
    instruction(0x108af9);push(m,a,o,0x108af9,a.gp[3]);
    call(m,a,o,s,0x108afa,0x17dff0);
    instruction(0x108aff);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x108b01);if(!(a.eflags&64u))goto restored_effect;
    instruction(0x108b03);a.gp[1]=read(m,a.gp[4]+0x1c);
    instruction(0x108b07);scalar(m,a,0,a.gp[4]+0x18);
    instruction(0x108b0d);push(m,a,o,0x108b0d,0);
    instruction(0x108b0f);push(m,a,o,0x108b0f,0);
    instruction(0x108b11);push(m,a,o,0x108b11,a.gp[1]);
    instruction(0x108b12);push(m,a,o,0x108b12,a.gp[3]);
    instruction(0x108b13);a.gp[2]=a.gp[5];
    instruction(0x108b15);a.gp[3]=0;iterator_logic_flags(r,0,4);
    call(m,a,o,s,0x108b17,0x179610);
    instruction(0x108b1c);a.gp[5]=pop(m,a);
    instruction(0x108b1d);a.gp[7]=pop(m,a);
    instruction(0x108b1e);a.gp[6]=pop(m,a);
    instruction(0x108b1f);a.gp[3]=pop(m,a);
    instruction(0x108b20);(void)pop(m,a);a.gp[4]+=12;return;
no_effect:
    instruction(0x108b23);a.gp[0]=a.gp[7];
    instruction(0x108b25);a.gp[7]=pop(m,a);
    instruction(0x108b26);a.gp[6]=pop(m,a);
    instruction(0x108b27);a.gp[3]=pop(m,a);
    instruction(0x108b28);(void)pop(m,a);a.gp[4]+=12;
}
// The iterator exposes arithmetic flags to its compiled callers. Preserve
// non-arithmetic flags and compute the defined x86 add/sub/logical results.
std::uint32_t iterator_flags(IteratorRegisters &a,std::uint32_t lhs,std::uint32_t rhs,unsigned width,bool subtract) {
    const auto mask=width==4?0xffffffffu:((1u<<(width*8u))-1u),sign=1u<<(width*8u-1u);
    lhs&=mask;rhs&=mask;const auto result=(subtract?lhs-rhs:lhs+rhs)&mask;
    const bool carry=subtract?lhs<rhs:std::uint64_t(lhs)+rhs>mask;
    const bool overflow=((subtract?(lhs^rhs):~(lhs^rhs))&(lhs^result)&sign)!=0;
    a.eflags=(a.eflags&~0x8d5u)|unsigned(carry)|((std::popcount(result&255u)%2u==0u)?4u:0u)|
        (((lhs^rhs^result)&16u)?16u:0u)|(result==0?64u:0u)|(result&sign?128u:0u)|(overflow?2048u:0u);
    return result;
}
void iterator_logic_flags(IteratorRegisters &a,std::uint32_t result,unsigned width) {
    const auto sign=1u<<(width*8u-1u);
    a.eflags=(a.eflags&~0x8d5u)|((std::popcount(result&255u)%2u==0u)?4u:0u)|
        (result==0?64u:0u)|(result&sign?128u:0u);
}
// 16E6A0 allocates the original datum pool through its complete raw ABI.
// Header/bitmap/record reads remain ordered even when they alias. The separate
// scalar DATA allocator shares the typed kernel; the legacy scratch-flags
// adapter remains separate.
void allocate_effect_datum(Memory &m,IteratorRegisters &a,const Observer &o,const Services &s) {
    if(halo2::native_effect_datum_allocate_detail::try_native(m,a.gp,a.eflags,s,o))return;
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};auto &r=a;
    const auto compare=[&](auto lhs,auto rhs,unsigned width=4){iterator_flags(r,lhs,rhs,width,true);};
    const auto signed_less=[&]{return bool(a.eflags&128u)!=bool(a.eflags&2048u);};
    const auto increment=[&](auto value,unsigned width=4){
        const auto carry=a.eflags&1u;const auto result=iterator_flags(r,value,1,width,false);a.eflags=(a.eflags&~1u)|carry;return result;
    };
    const auto shift_left=[&](std::uint32_t value,unsigned count){
        count&=31u;if(!count)return value;
        const auto result=value<<count;iterator_logic_flags(r,result,4);const auto carry=(value>>(32u-count))&1u;
        a.eflags=(a.eflags&~0x801u)|carry|((count==1u && ((result>>31u)^carry))?2048u:0u);return result;
    };
    const auto shift_right=[&](std::uint32_t value,unsigned count,bool arithmetic){
        const auto result=(value>>count)|(arithmetic && (value&0x80000000u)?~(none>>count):0u);
        iterator_logic_flags(r,result,4);a.eflags=(a.eflags&~0x801u)|((value>>(count-1u))&1u);return result;
    };
    instruction(0x16e6a0);push(m,a,o,0x16e6a0,a.gp[3]);
    instruction(0x16e6a1);push(m,a,o,0x16e6a1,a.gp[5]);
    instruction(0x16e6a2);push(m,a,o,0x16e6a2,a.gp[6]);
    instruction(0x16e6a3);a.gp[6]=read(m,a.gp[2]+0x34);
    instruction(0x16e6a6);push(m,a,o,0x16e6a6,a.gp[7]);
    instruction(0x16e6a7);a.gp[7]=read(m,a.gp[2]+0x38);
    instruction(0x16e6aa);a.gp[0]=none;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x16e6ad);compare(a.gp[6],a.gp[7]);
    instruction(0x16e6af);if(!signed_less())goto append;
    instruction(0x16e6b1);a.gp[3]=read(m,a.gp[2]+0x48);
scan:
    instruction(0x16e6b4);a.gp[1]=a.gp[6];
    instruction(0x16e6b6);a.gp[1]&=31u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x16e6b9);a.gp[5]=1;
    instruction(0x16e6be);a.gp[5]=shift_left(a.gp[5],a.gp[1]);
    instruction(0x16e6c0);a.gp[1]=a.gp[6];
    instruction(0x16e6c2);a.gp[1]=shift_right(a.gp[1],5,true);
    instruction(0x16e6c5);iterator_logic_flags(r,read(m,a.gp[3]+a.gp[1]*4u)&a.gp[5],4);
    instruction(0x16e6c8);if(a.eflags&64u)goto found;
    instruction(0x16e6ca);a.gp[1]=read(m,a.gp[2]+0x38);
    instruction(0x16e6cd);a.gp[6]=increment(a.gp[6]);
    instruction(0x16e6ce);compare(a.gp[6],a.gp[1]);
    instruction(0x16e6d0);if(signed_less())goto scan;
    instruction(0x16e6d2);goto append;
found:
    instruction(0x16e6d4);compare(a.gp[6],none);
    instruction(0x16e6d7);a.gp[3]=a.gp[6];
    instruction(0x16e6d9);if(!(a.eflags&64u))goto allocate;
append:
    instruction(0x16e6db);compare(a.gp[7],read(m,a.gp[2]+0x20));
    instruction(0x16e6de);if(!signed_less())goto done;
    instruction(0x16e6e0);compare(a.gp[7],none);
    instruction(0x16e6e3);a.gp[3]=a.gp[7];
    instruction(0x16e6e5);if(a.eflags&64u)goto done;
allocate:
    instruction(0x16e6e7);a.gp[6]=read(m,a.gp[2]+0x24);
    instruction(0x16e6ea);a.gp[0]=read(m,a.gp[2]+0x44);
    instruction(0x16e6ed);{
        const auto product=std::int64_t(std::bit_cast<std::int32_t>(a.gp[6]))*std::bit_cast<std::int32_t>(a.gp[3]);
        a.gp[6]=std::uint32_t(product);const auto overflow=product!=std::int64_t(std::bit_cast<std::int32_t>(a.gp[6]));a.eflags=(a.eflags&~0x801u)|(overflow?0x801u:0u);
    }
    instruction(0x16e6f0);a.gp[1]=read(m,a.gp[2]+0x48);
    instruction(0x16e6f3);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[0],4,false);
    instruction(0x16e6f5);a.gp[0]=a.gp[3];
    instruction(0x16e6f7);a.gp[0]=shift_right(a.gp[0],5,true);
    instruction(0x16e6fa);a.gp[5]=read(m,a.gp[1]+a.gp[0]*4u);
    instruction(0x16e6fd);a.gp[0]=a.gp[1]+a.gp[0]*4u;
    instruction(0x16e700);a.gp[1]=a.gp[3];
    instruction(0x16e702);a.gp[1]&=31u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x16e705);a.gp[7]=1;
    instruction(0x16e70a);a.gp[7]=shift_left(a.gp[7],a.gp[1]);
    instruction(0x16e70c);a.gp[5]|=a.gp[7];iterator_logic_flags(r,a.gp[5],4);
    instruction(0x16e70e);store(m,o,0x16e70e,a.gp[0],a.gp[5]);
    instruction(0x16e710);a.gp[7]=read(m,a.gp[2]+0x3c);
    instruction(0x16e713);a.gp[1]=read(m,a.gp[2]+0x38);
    instruction(0x16e716);a.gp[7]=increment(a.gp[7]);
    instruction(0x16e717);compare(a.gp[1],a.gp[3]);
    instruction(0x16e719);a.gp[0]=a.gp[3]+1u;
    instruction(0x16e71c);store(m,o,0x16e71c,a.gp[2]+0x3c,a.gp[7]);
    instruction(0x16e71f);store(m,o,0x16e71f,a.gp[2]+0x34,a.gp[0]);
    instruction(0x16e722);if(!signed_less() && !(a.eflags&64u))goto clear;
    instruction(0x16e724);store(m,o,0x16e724,a.gp[2]+0x38,a.gp[0]);
clear:
    instruction(0x16e727);a.gp[1]=read(m,a.gp[2]+0x24);
    instruction(0x16e72a);a.gp[5]=a.gp[1];
    instruction(0x16e72c);a.gp[1]=shift_right(a.gp[1],2,false);
    instruction(0x16e72f);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x16e731);a.gp[7]=a.gp[6];
    instruction(0x16e733);
    while(a.gp[1]) {
        store(m,o,0x16e733,a.gp[7],a.gp[0]);a.gp[7]+=(a.eflags&1024u)?std::uint32_t(-4):4u;--a.gp[1];
        if(a.gp[1])instruction(0x16e733);
    }
    instruction(0x16e735);a.gp[1]=a.gp[5];
    instruction(0x16e737);a.gp[1]&=3u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x16e73a);
    while(a.gp[1]) {
        store(m,o,0x16e73a,a.gp[7],a.gp[0],1);a.gp[7]+=(a.eflags&1024u)?none:1u;--a.gp[1];
        if(a.gp[1])instruction(0x16e73a);
    }
    instruction(0x16e73c);a.gp[0]=(a.gp[0]&0xffff0000u)|read(m,a.gp[2]+0x40,2);
    instruction(0x16e740);store(m,o,0x16e740,a.gp[6],a.gp[0],2);
    instruction(0x16e743);{
        const auto at=a.gp[2]+0x40,old=read(m,at,2),value=(old+1u)&65535u;
        SourcePhysicsContactRecordStore event{0x16e743,at,{},std::vector<std::byte>{std::byte(value),std::byte(value>>8u)}};
        if(o)event.before={std::byte(old),std::byte(old>>8u)};
        m.write(at,event.written);(void)increment(old,2);if(o)o(event);
    }
    instruction(0x16e747);compare(read(m,a.gp[2]+0x40,2),65535u,2);
    instruction(0x16e74c);if(!(a.eflags&64u))goto handle;
    instruction(0x16e74e);store(m,o,0x16e74e,a.gp[2]+0x40,0x8000,2);
handle:
    instruction(0x16e754);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[6],2)))));
    instruction(0x16e757);a.gp[0]=shift_left(a.gp[0],16);
    instruction(0x16e75a);a.gp[0]|=a.gp[3];iterator_logic_flags(r,a.gp[0],4);
done:
    instruction(0x16e75c);a.gp[7]=pop(m,a);
    instruction(0x16e75d);a.gp[6]=pop(m,a);
    instruction(0x16e75e);a.gp[5]=pop(m,a);
    instruction(0x16e75f);a.gp[3]=pop(m,a);
    instruction(0x16e760);(void)pop(m,a);
}
// 17DCE0 advances the actual cursor before reading the node word. Mode1
// accepts encoded first-person nodes, mode3 accepts every node, and all other
// signed low-word modes use WORLD admission (including the unbound FFFF node).
void next_effect_location(Memory &m,IteratorRegisters &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    instruction(0x17dce0);push(m,a,o,0x17dce0,a.gp[6]);
    instruction(0x17dce1);a.gp[6]=read(m,a.gp[4]+12);
    instruction(0x17dce5);a.gp[1]=read(m,a.gp[6]);
    instruction(0x17dce7);a.gp[0]=0;iterator_logic_flags(a,0,4);
    instruction(0x17dce9);iterator_flags(a,a.gp[1],none,4,true);
    instruction(0x17dcec);
    if(a.gp[1]!=none) {
        instruction(0x17dcee);a.gp[0]=read(m,0x4f7208);
        instruction(0x17dcf3);a.gp[1]&=65535u;iterator_logic_flags(a,a.gp[1],4);
        instruction(0x17dcf9);a.gp[1]*=0x3cu;
        // Low16*60 cannot overflow signed32. IMUL defines CF/OF only; the
        // following ADD defines every arithmetic flag before any use of them.
        a.eflags&=~0x801u;
        instruction(0x17dcfc);push(m,a,o,0x17dcfc,a.gp[3]);
        instruction(0x17dcfd);push(m,a,o,0x17dcfd,a.gp[7]);
        instruction(0x17dcfe);a.gp[1]=iterator_flags(a,a.gp[1],read(m,a.gp[0]+0x44),4,false);
        instruction(0x17dd01);a.gp[0]=a.gp[1];
        instruction(0x17dd03);a.gp[1]=read(m,a.gp[0]+4);
        instruction(0x17dd06);store(m,o,0x17dd06,a.gp[6],a.gp[1]);
        instruction(0x17dd08);a.gp[1]=(a.gp[1]&0xffff0000u)|read(m,a.gp[0]+2,2);
        instruction(0x17dd0c);iterator_flags(a,a.gp[1],65535u,2,true);
        const auto node=a.gp[1]&65535u;
        instruction(0x17dd10);
        if(node!=65535u) {
            instruction(0x17dd12);iterator_logic_flags(a,(node>>8u)&255u,1);
            instruction(0x17dd14);
        }
        if(node!=65535u && (node&0x8000u)) {
            instruction(0x17dd16);a.gp[3]=(a.gp[3]&0xffffff00u)|1u;instruction(0x17dd18);
        }else {
            instruction(0x17dd1a);a.gp[3]&=0xffffff00u;iterator_logic_flags(a,0,1);
        }
        instruction(0x17dd1c);a.gp[7]=read(m,a.gp[4]+24);
        instruction(0x17dd20);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[7]))));
        instruction(0x17dd23);a.gp[2]&=0xffffff00u;iterator_logic_flags(a,0,1);
        instruction(0x17dd25);
        const auto carry=a.eflags&1u;a.gp[1]=iterator_flags(a,a.gp[1],1,4,true);a.eflags=(a.eflags&~1u)|carry;
        bool unfiltered=false;
        instruction(0x17dd26);
        if(a.gp[1]==0) {instruction(0x17dd2f);a.gp[2]|=1u;}
        else {
            instruction(0x17dd28);a.gp[1]=iterator_flags(a,a.gp[1],2,4,true);
            instruction(0x17dd2b);unfiltered=a.gp[1]==0;
            if(!unfiltered)instruction(0x17dd2d);
        }
        if(!unfiltered) {
            instruction(0x17dd31);iterator_flags(a,a.gp[3],a.gp[2],1,true);
            instruction(0x17dd33);
            if((a.gp[3]&255u)!=(a.gp[2]&255u)) {
                instruction(0x17dd35);a.gp[2]=read(m,a.gp[4]+16);
                instruction(0x17dd39);push(m,a,o,0x17dd39,a.gp[7]);
                instruction(0x17dd3a);push(m,a,o,0x17dd3a,a.gp[6]);
                instruction(0x17dd3b);push(m,a,o,0x17dd3b,a.gp[2]);
                instruction(0x17dd3c);
                push(m,a,o,0x17dd3c,0x17dd41);next_effect_location(m,a,o,s);
            }
        }
        instruction(0x17dd41);a.gp[7]=pop(m,a);
        instruction(0x17dd42);a.gp[3]=pop(m,a);
    }
    instruction(0x17dd43);a.gp[6]=pop(m,a);
    instruction(0x17dd44);(void)pop(m,a);a.gp[4]+=12;
}
std::uint32_t state_product(IteratorRegisters &a,std::uint32_t left,std::uint32_t right) {
    const auto full=std::int64_t(std::bit_cast<std::int32_t>(left))*std::bit_cast<std::int32_t>(right);
    const auto result=std::uint32_t(full);const bool overflow=full!=std::int64_t(std::bit_cast<std::int32_t>(result));
    // IMUL leaves PF/AF/ZF/SF undefined; no source branch consumes them.
    a.eflags=(a.eflags&~0x801u)|(overflow?0x801u:0u);return result;
}
void state_scalar_compare(Abi &a,__m128 left,__m128 right,bool unordered) {
    SseEnvironment environment(a);unsigned char carry,zero,parity;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    if(unordered)asm volatile("ucomiss %4,%3; setb %0; sete %1; setp %2"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(left),"x"(right):"cc");
    else asm volatile("comiss %4,%3; setb %0; sete %1; setp %2"
        :"=qm"(carry),"=qm"(zero),"=qm"(parity):"x"(left),"x"(right):"cc");
#else
    throw std::runtime_error("Native effect state scalar comparison backend unavailable");
#endif
    a.eflags=(a.eflags&~0x8d5u)|unsigned(carry)|(unsigned(parity)<<2u)|(unsigned(zero)<<6u);
}
// Stage the original FSTP32 under the source x87 file, then restore the host's
// complete physical file. No compiler conversion can select another precision
// or lose C1/sticky flags. The Source ABI owns only its contiguous live lanes.
struct StateSingleSpill {std::uint32_t bits;std::uint16_t status;};
StateSingleSpill state_single_spill(const Abi &a) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER) && LDBL_MANT_DIG==64
    if(!a.floating_point || !a.x87_depth || a.x87_depth>8 || a.active_mmx ||
        (a.floating_point->x87_control_word&0x3fu)!=0x3fu || (a.x87_status&0x8080u))
        throw std::runtime_error("Native effect state requires its actual masked x87 file");
    alignas(16) std::array<std::byte,512> host{};
    alignas(16) std::array<std::byte,108> source{},result{};
    const auto cw=a.floating_point->x87_control_word;std::uint16_t tags=0xffffu;
    std::memcpy(source.data(),&cw,2);std::memcpy(source.data()+4,&a.x87_status,2);
    const auto top=(a.x87_status>>11u)&7u;
    for(unsigned n=0;n<a.x87_depth;++n) {
        std::array<std::byte,10> raw;std::memcpy(raw.data(),&a.x87[n],10);
        std::uint64_t significand{};std::uint16_t exponent{};
        std::memcpy(&significand,raw.data(),8);std::memcpy(&exponent,raw.data()+8,2);exponent&=0x7fffu;
        const unsigned tag=(!exponent && !significand)?1u:(!exponent || exponent==0x7fffu || !(significand>>63u))?2u:0u;
        const auto shift=((top+n)&7u)*2u;tags=std::uint16_t((tags&~(3u<<shift))|(tag<<shift));
        std::memcpy(source.data()+28+n*10,raw.data(),10);
    }
    std::memcpy(source.data()+8,&tags,2);StateSingleSpill output{};
#if defined(__x86_64__)
    asm volatile("fxsave64 %0\n\tfrstor %3\n\tfstps %1\n\tfnsave %2\n\tfxrstor64 %0"
#else
    asm volatile("fxsave %0\n\tfrstor %3\n\tfstps %1\n\tfnsave %2\n\tfxrstor %0"
#endif
        :"=m"(host),"=m"(output.bits),"=m"(result):"m"(source)
        :"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    std::memcpy(&output.status,result.data()+4,2);return output;
#else
    (void)a;throw std::runtime_error("Native effect state x87 store backend unavailable");
#endif
}
void state_store_single(Memory &m,Abi &a,const Observer &observe,std::uint32_t pc=0x17b1e1) {
    const auto result=state_single_spill(a);
    SourcePhysicsContactRecordStore event{pc,a.gp[6]+0x64,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);m.read(event.address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(result.bits>>(n*8));
    m.write(event.address,event.written);
    std::memmove(a.x87.data(),a.x87.data()+1,(a.x87_depth-1)*sizeof(long double));--a.x87_depth;a.x87_status=result.status;
    if(observe)observe(event);
}
// 17DC60 updates only the actual component chain's current/previous event.
// Generation checks are deliberately absent here, exactly as in this owner.
void reset_effect_components(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17dc60);a.gp[0]=read(m,a.gp[2]+0xfc);
    instruction(0x17dc66);iterator_flags(r,a.gp[0],none,4,true);instruction(0x17dc69);
    if(a.gp[0]!=none) {
        instruction(0x17dc6b);a.xmm[1]={};instruction(0x17dc6e);scalar(m,a,2,0x46a0d0);
        instruction(0x17dc76);push(m,a,o,0x17dc76,a.gp[5]);instruction(0x17dc77);push(m,a,o,0x17dc77,a.gp[6]);
        instruction(0x17dc78);a.gp[6]=read(m,0x51df74);instruction(0x17dc7e);
        do {
            instruction(0x17dc80);a.gp[1]=read(m,a.gp[6]+0x44);
            instruction(0x17dc83);a.gp[5]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[2]+0x5c,2)))));
            instruction(0x17dc87);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
            instruction(0x17dc8c);a.gp[0]=state_product(r,a.gp[0],0x54);
            instruction(0x17dc8f);a.gp[0]=iterator_flags(r,a.gp[0],a.gp[1],4,false);
            instruction(0x17dc91);a.gp[1]=read(m,a.gp[0]+0x14);
            instruction(0x17dc94);iterator_flags(r,a.gp[1],a.gp[5],4,true);instruction(0x17dc96);
            if(a.gp[1]==a.gp[5]) {
                instruction(0x17dc98);scalar(m,a,0,a.gp[2]+0x64);
                instruction(0x17dc9d);const auto flags=read(m,a.gp[0]+0xc,1)|1u;iterator_logic_flags(r,flags,1);store(m,o,0x17dc9d,a.gp[0]+0xc,flags,1);
                instruction(0x17dca1);state_scalar_compare(a,vector(a,0),vector(a,1),false);
                instruction(0x17dca4);store(m,o,0x17dca4,a.gp[0]+4,a.xmm[1][0]);instruction(0x17dca9);
                if(!(a.eflags&0x41u)) {
                    instruction(0x17dcab);a.xmm[3]=a.xmm[2];instruction(0x17dcae);
                    {SseEnvironment environment(a);vector(a,3,_mm_div_ss(vector(a,3),vector(a,0)));}
                    instruction(0x17dcb2);store(m,o,0x17dcb2,a.gp[0]+8,a.xmm[3][0]);instruction(0x17dcb7);
                }else {instruction(0x17dcb9);store(m,o,0x17dcb9,a.gp[0]+8,a.xmm[2][0]);instruction(0x17dcbe);}
            }else {
                instruction(0x17dcc0);a.gp[5]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[2]+0x5e,2)))));
                instruction(0x17dcc4);iterator_flags(r,a.gp[1],a.gp[5],4,true);instruction(0x17dcc6);
                if(a.gp[1]==a.gp[5]) {
                    instruction(0x17dcc8);const auto flags=read(m,a.gp[0]+0xc,1)&0xfeu;iterator_logic_flags(r,flags,1);store(m,o,0x17dcc8,a.gp[0]+0xc,flags,1);
                }
            }
            instruction(0x17dccc);a.gp[0]=read(m,a.gp[0]+0x38);
            instruction(0x17dccf);iterator_flags(r,a.gp[0],none,4,true);instruction(0x17dcd2);
        }while(a.gp[0]!=none);
        instruction(0x17dcd4);a.gp[6]=pop(m,a);instruction(0x17dcd5);a.gp[5]=pop(m,a);
    }
    instruction(0x17dcd6);(void)pop(m,a);
}
// 17B130 resets a live matching generation's selected event. The actual RNG
// owner and real bounded-real result remain required ABI services.
void reset_effect_event(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17b130);iterator_flags(r,a.gp[2],none,4,true);instruction(0x17b133);
    if(a.gp[2]==none){instruction(0x17b1ed);(void)pop(m,a);return;}
    instruction(0x17b139);a.gp[1]=read(m,0x4f720c);instruction(0x17b13f);a.gp[0]=a.gp[2];
    instruction(0x17b141);push(m,a,o,0x17b141,a.gp[6]);instruction(0x17b142);a.gp[6]=read(m,a.gp[1]+0x38);
    instruction(0x17b145);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17b14a);iterator_flags(r,a.gp[0],a.gp[6],4,true);instruction(0x17b14c);
    if(std::bit_cast<std::int32_t>(a.gp[0])>=std::bit_cast<std::int32_t>(a.gp[6]))goto done;
    instruction(0x17b152);a.gp[6]=read(m,a.gp[1]+0x24);instruction(0x17b155);a.gp[6]=state_product(r,a.gp[6],a.gp[0]);
    instruction(0x17b158);a.gp[6]=iterator_flags(r,a.gp[6],read(m,a.gp[1]+0x44),4,false);
    instruction(0x17b15b);a.gp[0]=(a.gp[0]&0xffff0000u)|read(m,a.gp[6],2);
    instruction(0x17b15e);iterator_logic_flags(r,a.gp[0]&65535u,2);instruction(0x17b161);if(!(a.gp[0]&65535u))goto done;
    instruction(0x17b167);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[0]))));
    instruction(0x17b16a);{const auto old=a.gp[2];a.gp[2]=(old>>16u)|((old&0x80000000u)?0xffff0000u:0u);iterator_logic_flags(r,a.gp[2],4);a.eflags|=(old>>15u)&1u;}
    instruction(0x17b16d);iterator_flags(r,a.gp[0],a.gp[2],4,true);instruction(0x17b16f);if(a.gp[0]!=a.gp[2])goto done;
    instruction(0x17b171);a.gp[1]=read(m,a.gp[6]+4);instruction(0x17b174);a.gp[2]=read(m,0x4f0414);
    instruction(0x17b17a);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17b180);a.gp[1]<<=4u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17b183);iterator_logic_flags(r,a.gp[7]&65535u,2);
    instruction(0x17b186);a.gp[1]=read(m,a.gp[1]+a.gp[2]+8);instruction(0x17b18a);
    if(a.eflags&0x80u)goto done;
    instruction(0x17b18c);a.gp[2]=read(m,a.gp[1]+0x14);
    instruction(0x17b18f);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[7]))));
    instruction(0x17b192);iterator_flags(r,a.gp[0],a.gp[2],4,true);instruction(0x17b194);
    if(std::bit_cast<std::int32_t>(a.gp[0])>=std::bit_cast<std::int32_t>(a.gp[2]))goto done;
    instruction(0x17b196);a.gp[2]=read(m,a.gp[1]+0x18);instruction(0x17b199);a.gp[0]=state_product(r,a.gp[0],0x38);
    instruction(0x17b19c);{const auto flags=read(m,a.gp[6]+2,1)&0xfeu;iterator_logic_flags(r,flags,1);store(m,o,0x17b19c,a.gp[6]+2,flags,1);}
    instruction(0x17b1a0);a.xmm[0]={};instruction(0x17b1a3);a.gp[0]=iterator_flags(r,a.gp[0],a.gp[2],4,false);
    instruction(0x17b1a5);a.gp[1]=a.gp[0];instruction(0x17b1a7);store(m,o,0x17b1a7,a.gp[6]+0x5c,a.gp[7],2);
    instruction(0x17b1ab);store(m,o,0x17b1ab,a.gp[6]+0x60,a.xmm[0][0]);instruction(0x17b1b0);scalar(m,a,0,a.gp[1]+8);
    instruction(0x17b1b5);state_scalar_compare(a,vector(a,0),_mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(read(m,a.gp[1]+12)))),true);
    instruction(0x17b1b9);a.gp[0]=(a.gp[0]&0xffff00ffu)|(((a.eflags&0xd5u)|2u)<<8u);
    instruction(0x17b1ba);iterator_logic_flags(r,(a.gp[0]>>8u)&0x44u,1);
    instruction(0x17b1bd);a.gp[0]=read(m,a.gp[1]+8);instruction(0x17b1c0);
    if(!(a.eflags&4u)) {
        instruction(0x17b1c2);store(m,o,0x17b1c2,a.gp[6]+0x64,a.gp[0]);instruction(0x17b1c5);a.gp[2]=a.gp[6];
        instruction(0x17b1c7);a.gp[6]=pop(m,a);instruction(0x17b1c8);reset_effect_components(m,a,o,s);return;
    }
    instruction(0x17b1cd);a.gp[2]=read(m,a.gp[1]+12);instruction(0x17b1d0);push(m,a,o,0x17b1d0,a.gp[2]);
    instruction(0x17b1d1);push(m,a,o,0x17b1d1,a.gp[0]);instruction(0x17b1d2);a.gp[0]=read(m,a.gp[6]+4);
    call(m,a,o,s,0x17b1d5,0x17aab0);instruction(0x17b1da);a.gp[1]=a.gp[0];call(m,a,o,s,0x17b1dc,0x259b0);
    instruction(0x17b1e1);state_store_single(m,a,o);instruction(0x17b1e4);a.gp[2]=a.gp[6];
    instruction(0x17b1e6);a.gp[6]=pop(m,a);instruction(0x17b1e7);reset_effect_components(m,a,o,s);return;
done:
    instruction(0x17b1ec);a.gp[6]=pop(m,a);instruction(0x17b1ed);(void)pop(m,a);
}
// Commit RMW flags with the write, including when a memory write fails.
void and_effect_byte(Memory &m,Abi &a,const Observer &o,std::uint32_t pc,
    std::uint32_t at,std::uint32_t mask) {
    const auto value=read(m,at,1)&mask;
    SourcePhysicsContactRecordStore event{pc,at,{},std::vector<std::byte>{std::byte(value)}};
    if(o){event.before.resize(1);m.read(at,event.before);}
    m.write(at,event.written);IteratorRegisters r{a.gp,a.eflags};iterator_logic_flags(r,value,1);
    if(o)o(event);
}
// 17A1A0 restarts the existing instance and conditionally starts its authored
// looping sound. Reset, location and sound allocation retain their real owners.
void restart_effect(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17a1a0);a.gp[0]=read(m,0x4f720c);
    instruction(0x17a1a5);push(m,a,o,0x17a1a5,a.gp[3]);
    instruction(0x17a1a6);a.gp[2]=read(m,0x4f0414);
    instruction(0x17a1ac);push(m,a,o,0x17a1ac,a.gp[5]);
    instruction(0x17a1ad);a.gp[5]=read(m,a.gp[4]+12);
    instruction(0x17a1b1);a.gp[3]=a.gp[5];
    instruction(0x17a1b3);a.gp[3]&=65535u;iterator_logic_flags(r,a.gp[3],4);
    instruction(0x17a1b9);a.gp[3]*=0x190u;a.eflags&=~0x801u;
    instruction(0x17a1bf);push(m,a,o,0x17a1bf,a.gp[6]);
    instruction(0x17a1c0);push(m,a,o,0x17a1c0,a.gp[7]);
    instruction(0x17a1c1);a.gp[7]=read(m,a.gp[0]+0x44);
    instruction(0x17a1c4);a.gp[1]=read(m,a.gp[3]+a.gp[7]+4);
    instruction(0x17a1c8);a.gp[3]=iterator_flags(r,a.gp[3],a.gp[7],4,false);
    instruction(0x17a1ca);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17a1d0);a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);a.eflags&=~0x801u;
    instruction(0x17a1d3);a.gp[6]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0x17a1d7);and_effect_byte(m,a,o,0x17a1d7,a.gp[3]+2,0xfb);
    instruction(0x17a1db);a.gp[7]=0;iterator_logic_flags(r,0,4);
    instruction(0x17a1dd);a.gp[2]=a.gp[5];
    call(m,a,o,s,0x17a1df,0x17b130);
    instruction(0x17a1e4);a.gp[0]=read(m,a.gp[6]+0x20);
    instruction(0x17a1e7);a.gp[1]=none;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17a1ea);iterator_flags(r,a.gp[0],a.gp[1],4,true);
    instruction(0x17a1ec);if(a.eflags&64u)goto done;
    instruction(0x17a1ee);a.gp[0]=(a.gp[0]&0xffff0000u)|read(m,a.gp[6]+0x24,2);
    instruction(0x17a1f2);iterator_flags(r,a.gp[0],a.gp[1],2,true);
    instruction(0x17a1f5);if(a.eflags&64u)goto done;
    instruction(0x17a1f7);iterator_flags(r,read(m,a.gp[3]+8),a.gp[1],4,true);
    instruction(0x17a1fa);if(!(a.eflags&64u))goto done;
    instruction(0x17a1fc);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[0]))));
    instruction(0x17a1ff);a.gp[1]=read(m,a.gp[3]+a.gp[0]*4u+0x7c);
    instruction(0x17a203);push(m,a,o,0x17a203,3);
    instruction(0x17a205);a.gp[2]=a.gp[4]+0x18;
    instruction(0x17a209);push(m,a,o,0x17a209,a.gp[2]);
    instruction(0x17a20a);push(m,a,o,0x17a20a,a.gp[3]);
    instruction(0x17a20b);store(m,o,0x17a20b,a.gp[4]+0x20,a.gp[1]);
    call(m,a,o,s,0x17a20f,0x17dce0);
    instruction(0x17a214);iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17a216);if(a.eflags&64u)goto done;
    instruction(0x17a218);a.gp[6]=read(m,a.gp[6]+0x20);
    instruction(0x17a21b);push(m,a,o,0x17a21b,a.gp[5]);
    call(m,a,o,s,0x17a21c,0x18d540);
    instruction(0x17a221);store(m,o,0x17a221,a.gp[3]+8,a.gp[0]);
done:
    instruction(0x17a224);a.gp[7]=pop(m,a);
    instruction(0x17a225);a.gp[6]=pop(m,a);
    instruction(0x17a226);a.gp[5]=pop(m,a);
    instruction(0x17a227);a.gp[3]=pop(m,a);
    instruction(0x17a228);(void)pop(m,a);a.gp[4]+=4;
}
// Refresh only a live matching generation/definition. The original return is
// AL, not a normalized EAX: callers and stopped-owner checks retain its high bits.
void refresh_effect(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17dff0);push(m,a,o,0x17dff0,a.gp[3]);
    instruction(0x17dff1);push(m,a,o,0x17dff1,a.gp[6]);
    instruction(0x17dff2);a.gp[6]=0;iterator_logic_flags(r,0,4);
    instruction(0x17dff4);iterator_flags(r,a.gp[7],none,4,true);
    instruction(0x17dff7);if(a.eflags&64u)goto no_handle;
    instruction(0x17dff9);a.gp[2]=read(m,0x4f720c);
    instruction(0x17dfff);a.gp[0]=read(m,a.gp[2]+0x38);
    instruction(0x17e002);a.gp[1]=a.gp[7];
    instruction(0x17e004);a.gp[1]&=65535u;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17e00a);iterator_flags(r,a.gp[1],a.gp[0],4,true);
    instruction(0x17e00c);if(bool(a.eflags&128u)==bool(a.eflags&2048u))goto no_handle;
    instruction(0x17e00e);a.gp[0]=read(m,a.gp[2]+0x24);
    instruction(0x17e011);a.gp[3]=read(m,a.gp[2]+0x44);
    instruction(0x17e014);{
        const auto product=std::int64_t(std::bit_cast<std::int32_t>(a.gp[0]))*std::int64_t(std::bit_cast<std::int32_t>(a.gp[1]));
        a.gp[0]=std::uint32_t(product);const bool overflow=product!=std::int64_t(std::bit_cast<std::int32_t>(a.gp[0]));a.eflags=(a.eflags&~0x801u)|(overflow?0x801u:0u);
    }
    instruction(0x17e017);a.gp[1]=(a.gp[1]&0xffff0000u)|read(m,a.gp[0]+a.gp[3],2);
    instruction(0x17e01b);a.gp[0]=iterator_flags(r,a.gp[0],a.gp[3],4,false);
    instruction(0x17e01d);iterator_logic_flags(r,a.gp[1]&65535u,2);
    instruction(0x17e020);if(a.eflags&64u)goto no_handle;
    instruction(0x17e022);a.gp[2]=a.gp[7];
    instruction(0x17e024);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[1]))));
    instruction(0x17e027);{const auto value=a.gp[2];a.gp[2]=(value>>16u)|((value&0x80000000u)?0xffff0000u:0u);iterator_logic_flags(r,a.gp[2],4);a.eflags=(a.eflags&~0x801u)|((value>>15u)&1u);}
    instruction(0x17e02a);iterator_flags(r,a.gp[1],a.gp[2],4,true);
    instruction(0x17e02c);if(!(a.eflags&64u))goto no_handle;
    instruction(0x17e02e);a.gp[6]=a.gp[0];
no_handle:
    instruction(0x17e030);a.gp[0]&=0xffffff00u;iterator_logic_flags(r,0,1);
    instruction(0x17e032);iterator_logic_flags(r,a.gp[6],4);
    instruction(0x17e034);if(a.eflags&64u)goto done;
    instruction(0x17e036);a.gp[2]=read(m,a.gp[6]+4);
    instruction(0x17e039);iterator_flags(r,a.gp[2],read(m,a.gp[4]+12),4,true);
    instruction(0x17e03d);if(!(a.eflags&64u))goto done;
    instruction(0x17e03f);and_effect_byte(m,a,o,0x17e03f,a.gp[6]+2,0xbf);
    instruction(0x17e043);push(m,a,o,0x17e043,a.gp[7]);
    instruction(0x17e044);a.gp[3]=(a.gp[3]&0xffffff00u)|1u;
    call(m,a,o,s,0x17e046,0x17a1a0);
    instruction(0x17e04b);a.gp[0]=(a.gp[0]&0xffffff00u)|(a.gp[3]&255u);
done:
    instruction(0x17e04d);a.gp[6]=pop(m,a);
    instruction(0x17e04e);a.gp[3]=pop(m,a);
    instruction(0x17e04f);(void)pop(m,a);a.gp[4]+=4;
}
// Build the authored attached-effect request in its original stack storage.
// Object location, direction selection and allocation retain their real owners.
void create_attached_effect(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x179610);a.gp[4]=iterator_flags(r,a.gp[4],0xa4,4,true);
    instruction(0x179616);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x179618);push(m,a,o,0x179618,a.gp[6]);
    instruction(0x179619);push(m,a,o,0x179619,a.gp[7]);
    instruction(0x17961a);a.gp[1]=24;
    instruction(0x17961f);a.gp[7]=a.gp[4]+8;
    while(a.gp[1]) {
        instruction(0x179623);store(m,o,0x179623,a.gp[7],a.gp[0]);
        a.gp[7]+=(a.eflags&1024u)?std::uint32_t(-4):4u;--a.gp[1];
    }
    instruction(0x179625);a.gp[1]=read(m,a.gp[4]+8);
    instruction(0x179629);a.gp[0]=none;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17962c);store(m,o,0x17962c,a.gp[4]+0x20,a.gp[0],2);
    instruction(0x179631);store(m,o,0x179631,a.gp[4]+0x18,a.gp[0]);
    instruction(0x179635);store(m,o,0x179635,a.gp[4]+0x14,a.gp[0]);
    instruction(0x179639);store(m,o,0x179639,a.gp[4]+0x1c,a.gp[0],2);
    instruction(0x17963e);a.gp[0]=0xff808080;
    instruction(0x179643);store(m,o,0x179643,a.gp[4]+0x5c,a.gp[0]);
    instruction(0x179647);store(m,o,0x179647,a.gp[4]+0x60,a.gp[0]);
    instruction(0x17964b);a.gp[0]=read(m,a.gp[4]+0xb0);
    instruction(0x179652);store(m,o,0x179652,a.gp[4]+12,a.gp[0]);
    instruction(0x179656);a.gp[0]=read(m,a.gp[4]+0xbc);
    instruction(0x17965d);a.gp[1]|=5;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x179660);a.gp[7]=0;iterator_logic_flags(r,0,4);
    instruction(0x179662);store(m,o,0x179662,a.gp[4]+8,a.gp[1]);
    instruction(0x179666);a.gp[1]=read(m,a.gp[4]+0xb8);
    instruction(0x17966d);store(m,o,0x17966d,a.gp[4]+0x48,a.xmm[0][0]);
    instruction(0x179673);scalar(m,a,0,a.gp[4]+0xb4);
    instruction(0x17967c);store(m,o,0x17967c,a.gp[4]+0x54,a.gp[0]);
    instruction(0x179680);a.gp[6]=a.gp[4]+0x68;
    instruction(0x179684);a.gp[0]=a.gp[2];
    instruction(0x179686);store(m,o,0x179686,a.gp[4]+0x3c,a.gp[7]);
    instruction(0x17968a);store(m,o,0x17968a,a.gp[4]+0x40,a.gp[7]);
    instruction(0x17968e);store(m,o,0x17968e,a.gp[4]+0x44,a.gp[7],2);
    instruction(0x179693);store(m,o,0x179693,a.gp[4]+0x38,a.gp[7]);
    instruction(0x179697);store(m,o,0x179697,a.gp[4]+0x64,a.gp[7]);
    instruction(0x17969b);store(m,o,0x17969b,a.gp[4]+0x10,a.gp[2]);
    instruction(0x17969f);store(m,o,0x17969f,a.gp[4]+0x4c,a.xmm[0][0]);
    instruction(0x1796a5);store(m,o,0x1796a5,a.gp[4]+0x50,a.gp[1]);
    call(m,a,o,s,0x1796a9,0xbbe00);
    instruction(0x1796ae);a.gp[0]=a.gp[4]+0x74;
    instruction(0x1796b2);a.gp[1]=a.gp[6];
    call(m,a,o,s,0x1796b4,0x1791c0);
    instruction(0x1796b9);iterator_flags(r,a.gp[3],a.gp[7],4,true);
    instruction(0x1796bb);a.gp[1]=a.gp[0];
    instruction(0x1796bd);a.gp[7]=pop(m,a);
    instruction(0x1796be);store(m,o,0x1796be,a.gp[4]+0x20,a.gp[1]);
    instruction(0x1796c2);store(m,o,0x1796c2,a.gp[4]+0x24,2);
    instruction(0x1796ca);a.gp[6]=pop(m,a);
    instruction(0x1796cb);if(a.eflags&64u)goto request_ready;
    instruction(0x1796cd);a.gp[2]=read(m,a.gp[3]);
    instruction(0x1796cf);a.gp[0]=read(m,a.gp[3]+4);
    instruction(0x1796d2);a.gp[1]=read(m,a.gp[3]+8);
    instruction(0x1796d5);store(m,o,0x1796d5,a.gp[4]+12,a.gp[2]);
    instruction(0x1796d9);store(m,o,0x1796d9,a.gp[4]+0x10,a.gp[0]);
    instruction(0x1796dd);store(m,o,0x1796dd,a.gp[4]+0x14,a.gp[1]);
request_ready:
    instruction(0x1796e1);a.gp[2]=a.gp[4];
    instruction(0x1796e4);push(m,a,o,0x1796e4,a.gp[2]);
    call(m,a,o,s,0x1796e5,0x178e30);
    instruction(0x1796ea);a.gp[4]=iterator_flags(r,a.gp[4],0xa4,4,false);
    instruction(0x1796f0);(void)pop(m,a);a.gp[4]+=16;
}
// 18BE00 selects the original local sound policy from the sound flag and the
// owner's authored type byte. Preserve all upper byte-register contents.
void sound_is_local(Memory &m,IteratorRegisters &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    instruction(0x18be00);a.gp[0]&=0xffffff00u;iterator_logic_flags(a,0,1);
    instruction(0x18be02);iterator_flags(a,a.gp[2],none,4,true);
    instruction(0x18be05);if(a.eflags&64u)goto done;
    instruction(0x18be07);push(m,a,o,0x18be07,a.gp[3]);
    instruction(0x18be08);a.gp[2]&=65535u;iterator_logic_flags(a,a.gp[2],4);
    instruction(0x18be0e);push(m,a,o,0x18be0e,a.gp[6]);
    instruction(0x18be0f);a.gp[6]=read(m,0x4f0414);
    instruction(0x18be15);{const auto old=a.gp[2];a.gp[2]<<=4;iterator_logic_flags(a,a.gp[2],4);a.eflags=(a.eflags&~0x801u)|((old>>28u)&1u);}
    instruction(0x18be18);a.gp[2]=read(m,a.gp[2]+a.gp[6]+8);
    instruction(0x18be1c);iterator_logic_flags(a,read(m,a.gp[2],1)&64u,1);
    instruction(0x18be1f);if(a.eflags&64u)goto restore;
    instruction(0x18be21);iterator_flags(a,a.gp[1],none,4,true);
    instruction(0x18be24);if(a.eflags&64u)goto restore;
    instruction(0x18be26);a.gp[2]=read(m,0x4ecbd0);
    instruction(0x18be2c);a.gp[2]=read(m,a.gp[2]+0x44);
    instruction(0x18be2f);a.gp[1]&=65535u;iterator_logic_flags(a,a.gp[1],4);
    instruction(0x18be35);a.gp[1]*=3u;
    instruction(0x18be38);a.gp[1]=(a.gp[1]&0xffffff00u)|read(m,a.gp[2]+a.gp[1]*4u+3u,1);
    instruction(0x18be3c);a.gp[3]=1;
    instruction(0x18be41);{
        const auto count=a.gp[1]&31u;
        if(count){a.gp[3]<<=count;iterator_logic_flags(a,a.gp[3],4);a.eflags&=~0x801u;}
    }
    instruction(0x18be43);iterator_logic_flags(a,a.gp[3]&4u,1);
    instruction(0x18be46);if(a.eflags&64u)goto restore;
    instruction(0x18be48);a.gp[0]=(a.gp[0]&0xffffff00u)|1u;
restore:
    instruction(0x18be4a);a.gp[6]=pop(m,a);
    instruction(0x18be4b);a.gp[3]=pop(m,a);
done:
    instruction(0x18be4c);(void)pop(m,a);
}
void direct_sound(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x18be50);a.gp[4]=iterator_flags(r,a.gp[4],0x28,4,true);
    instruction(0x18be53);a.gp[2]=read(m,a.gp[4]+0x30);
    instruction(0x18be57);push(m,a,o,0x18be57,a.gp[5]);
    instruction(0x18be58);a.gp[5]=read(m,a.gp[4]+0x30);
    instruction(0x18be5c);push(m,a,o,0x18be5c,a.gp[6]);
    instruction(0x18be5d);push(m,a,o,0x18be5d,a.gp[7]);
    instruction(0x18be5e);push(m,a,o,0x18be5e,a.gp[5]);
    instruction(0x18be5f);push(m,a,o,0x18be5f,a.gp[3]);
    instruction(0x18be60);a.gp[6]=a.gp[4]+0x14;
    instruction(0x18be64);store(m,o,0x18be64,a.gp[4]+0x15,0,1);
    call(m,a,o,s,0x18be69,0x18c090);
    instruction(0x18be6e);a.gp[1]=a.gp[5];
    instruction(0x18be70);a.gp[2]=a.gp[3];
    call(m,a,o,s,0x18be72,0x18be00);
    instruction(0x18be77);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x18be79);a.gp[7]=0x44ec14;
    instruction(0x18be7e);
    if(a.eflags&64u){instruction(0x18be80);a.gp[7]=0x44ebf4;}
    instruction(0x18be85);a.gp[0]=read(m,a.gp[4]+0x40);
    instruction(0x18be89);push(m,a,o,0x18be89,0);
    instruction(0x18be8b);push(m,a,o,0x18be8b,none);
    instruction(0x18be8d);push(m,a,o,0x18be8d,a.gp[0]);
    instruction(0x18be8e);a.gp[1]=a.gp[4]+0x18;
    instruction(0x18be92);push(m,a,o,0x18be92,a.gp[1]);
    instruction(0x18be93);push(m,a,o,0x18be93,1);
    instruction(0x18be95);push(m,a,o,0x18be95,a.gp[3]);
    instruction(0x18be96);a.gp[6]=a.gp[5];
    call(m,a,o,s,0x18be98,0x18c130);
    instruction(0x18be9d);a.gp[7]=pop(m,a);
    instruction(0x18be9e);a.gp[6]=pop(m,a);
    instruction(0x18be9f);a.gp[5]=pop(m,a);
    instruction(0x18bea0);a.gp[4]=iterator_flags(r,a.gp[4],0x28,4,false);
    instruction(0x18bea3);(void)pop(m,a);a.gp[4]+=12;
}
// CABC0's forced zoom-out tail uses this unowned request. Its unused bytes
// remain untouched just as in the original; the complete lower request owner
// consumes the exact control bits and sound datum.
void unowned_zoom_sound(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x18c4b0);a.gp[4]=iterator_flags(r,a.gp[4],0x58,4,true);
    instruction(0x18c4b3);a.gp[0]=0;iterator_logic_flags(r,0,4);
    instruction(0x18c4b5);store(m,o,0x18c4b5,a.gp[4]+2,a.gp[0]&255u,1);
    instruction(0x18c4b9);store(m,o,0x18c4b9,a.gp[4],a.gp[0]&65535u,2);
    instruction(0x18c4bd);store(m,o,0x18c4bd,a.gp[4]+3,a.gp[0]&255u,1);
    instruction(0x18c4c1);store(m,o,0x18c4c1,a.gp[4]+8,a.gp[0]);
    instruction(0x18c4c5);store(m,o,0x18c4c5,a.gp[4]+0x50,a.gp[0]);
    instruction(0x18c4c9);store(m,o,0x18c4c9,a.gp[4]+0x4c,a.gp[0]);
    instruction(0x18c4cd);store(m,o,0x18c4cd,a.gp[4]+0x54,a.gp[0]);
    instruction(0x18c4d1);a.gp[0]=read(m,a.gp[4]+0x5c);
    instruction(0x18c4d5);a.gp[1]=none;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x18c4d8);push(m,a,o,0x18c4d8,a.gp[0]);
    instruction(0x18c4d9);a.gp[0]=a.gp[4]+4;
    instruction(0x18c4dd);store(m,o,0x18c4dd,a.gp[4]+8,a.xmm[0][0]);
    instruction(0x18c4e3);store(m,o,0x18c4e3,a.gp[4]+0x4c,a.gp[1]);
    instruction(0x18c4e7);store(m,o,0x18c4e7,a.gp[4]+0x48,a.gp[1]);
    call(m,a,o,s,0x18c4eb,0x18cdd0);
    instruction(0x18c4f0);a.gp[4]=iterator_flags(r,a.gp[4],0x58,4,false);
    instruction(0x18c4f3);(void)pop(m,a);a.gp[4]+=4;
}
// CABC0: retain the true reset stores, local camera reset and local sound
// gate. The optional publication observes committed reset input at CAC96;
// it never changes the producer's locality, caller ABI or sound outcome.
// 17B0D0 stores the authored scalar pair and placement vectors. CMOV's
// default-pointer operand is read eagerly, and vector reads stay interleaved
// with their stores so overlapping request/body records retain source order.
void initialize_effect_placement(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17b0d0);push(m,a,o,0x17b0d0,a.gp[3]);
    instruction(0x17b0d1);store(m,o,0x17b0d1,a.gp[0]+0x6c,a.xmm[0][0]);
    instruction(0x17b0d6);scalar(m,a,0,a.gp[4]+8);
    instruction(0x17b0dc);iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17b0de);const auto fallback=read(m,0x474c70);
    if(a.eflags&64u)a.gp[1]=fallback;
    instruction(0x17b0e5);iterator_logic_flags(r,a.gp[2],4);
    instruction(0x17b0e7);push(m,a,o,0x17b0e7,a.gp[7]);
    instruction(0x17b0e8);store(m,o,0x17b0e8,a.gp[0]+0x70,a.xmm[0][0]);
    instruction(0x17b0ed);a.gp[3]=read(m,a.gp[1]);
    instruction(0x17b0ef);a.gp[7]=a.gp[0]+0x24;
    instruction(0x17b0f2);store(m,o,0x17b0f2,a.gp[7],a.gp[3]);
    instruction(0x17b0f4);a.gp[3]=read(m,a.gp[1]+4);
    instruction(0x17b0f7);store(m,o,0x17b0f7,a.gp[7]+4,a.gp[3]);
    instruction(0x17b0fa);a.gp[1]=read(m,a.gp[1]+8);
    instruction(0x17b0fd);store(m,o,0x17b0fd,a.gp[7]+8,a.gp[1]);
    instruction(0x17b100);a.gp[7]=pop(m,a);
    instruction(0x17b101);a.gp[3]=pop(m,a);
    instruction(0x17b102);
    if(!(a.eflags&64u)) {
        instruction(0x17b104);a.gp[1]=read(m,a.gp[2]);
        instruction(0x17b106);a.gp[0]=iterator_flags(r,a.gp[0],0x3cu,4,false);
        instruction(0x17b109);store(m,o,0x17b109,a.gp[0],a.gp[1]);
        instruction(0x17b10b);a.gp[1]=read(m,a.gp[2]+4);
        instruction(0x17b10e);store(m,o,0x17b10e,a.gp[0]+4,a.gp[1]);
        instruction(0x17b111);a.gp[2]=read(m,a.gp[2]+8);
        instruction(0x17b114);store(m,o,0x17b114,a.gp[0]+8,a.gp[2]);
        instruction(0x17b117);(void)pop(m,a);a.gp[4]+=4;
    }else {
        instruction(0x17b11a);store(m,o,0x17b11a,a.gp[0]+0x40,0);
        instruction(0x17b121);store(m,o,0x17b121,a.gp[0]+0x44,0);
        instruction(0x17b128);(void)pop(m,a);a.gp[4]+=4;
    }
}
// 17E5E0 initializes exactly32 handles in ascending source-store order. The
// loop is native bookkeeping: it never changes the caller's registers/flags.
void initialize_effect_handles(Memory &m,Abi &a,const Observer &o,const Services &s) {
    for(unsigned slot=0;slot<32;++slot) {
        const auto pc=slot==0?0x17e5e0u:0x17e5e2u+(slot-1u)*3u;if(s.instruction)s.instruction(pc);
        store(m,o,pc,a.gp[1]+slot*4u,a.gp[0]);
    }
    if(s.instruction)s.instruction(0x17e63f);
    (void)pop(m,a);
}
// 17B960 resets sixteen location pairs while retaining the source ADD/DEC
// flags, including DEC's preserved carry and a stopped store's exact prefix.
void initialize_effect_locations(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17b960);a.gp[0]=iterator_flags(r,a.gp[0],0x110u,4,false);
    instruction(0x17b965);a.gp[1]=16;
    instruction(0x17b96a);a.gp[2]=none;iterator_logic_flags(r,a.gp[2],4);
    instruction(0x17b96d);
    do {
        instruction(0x17b970);store(m,o,0x17b970,a.gp[0],0);
        instruction(0x17b976);store(m,o,0x17b976,a.gp[0]+4,a.gp[2]);
        instruction(0x17b979);a.gp[0]=iterator_flags(r,a.gp[0],8,4,false);
        instruction(0x17b97c);const auto carry=a.eflags&1u;
        a.gp[1]=iterator_flags(r,a.gp[1],1,4,true);a.eflags=(a.eflags&~1u)|carry;
        instruction(0x17b97d);
    }while(!(a.eflags&64u));
    instruction(0x17b97f);(void)pop(m,a);
}
// 17AFB0 admits an authored effect, retains actual allocation/reclamation
// results, initializes its source record and enters the real reset owners.
void allocate_effect(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    const auto logic=[&](std::uint32_t value,unsigned width=4){iterator_logic_flags(r,value,width);return value;};
    const auto compare=[&](std::uint32_t lhs,std::uint32_t rhs){iterator_flags(r,lhs,rhs,4,true);};
    const auto right_two=[&](std::uint32_t value) {
        const auto result=value>>2u;iterator_logic_flags(r,result,4);a.eflags=(a.eflags&~0x801u)|((value>>1u)&1u);return result;
    };
    instruction(0x17afb0);push(m,a,o,0x17afb0,a.gp[7]);
    instruction(0x17afb1);a.gp[7]=a.gp[0];
    instruction(0x17afb3);a.gp[0]=logic(none);
    instruction(0x17afb6);compare(a.gp[7],none);
    instruction(0x17afb9);if(a.eflags&64u)goto final_return;
    instruction(0x17afbf);a.gp[2]=read(m,0x4f0414);
    instruction(0x17afc5);a.gp[1]=a.gp[7];
    instruction(0x17afc7);a.gp[1]=logic(a.gp[1]&65535u);
    instruction(0x17afcd);a.gp[1]<<=4u;iterator_logic_flags(r,a.gp[1],4);a.eflags&=~0x801u;
    instruction(0x17afd0);push(m,a,o,0x17afd0,a.gp[6]);
    instruction(0x17afd1);a.gp[6]=read(m,a.gp[1]+a.gp[2]+8);
    instruction(0x17afd5);a.gp[1]=(a.gp[1]&0xffffff00u)|read(m,a.gp[4]+0x10,1);
    instruction(0x17afd9);logic(a.gp[1]&255u,1);
    instruction(0x17afdb);if(!(a.eflags&64u))goto admitted;
    instruction(0x17afdd);a.gp[1]=read(m,a.gp[6]);
    instruction(0x17afdf);a.gp[1]=right_two(a.gp[1]);
    instruction(0x17afe2);logic(a.gp[1]&1u,1);
    instruction(0x17afe5);if(!(a.eflags&64u))goto restore_source;
admitted:
    instruction(0x17afeb);a.gp[1]=read(m,a.gp[6]+0x14);
    instruction(0x17afee);logic(a.gp[1]);
    instruction(0x17aff0);if(std::bit_cast<std::int32_t>(a.gp[1])<=0)goto restore_source;
    instruction(0x17aff6);push(m,a,o,0x17aff6,a.gp[3]);
    instruction(0x17aff7);push(m,a,o,0x17aff7,a.gp[5]);
    instruction(0x17aff8);a.gp[5]=read(m,0x4f720c);
    instruction(0x17affe);a.gp[2]=a.gp[5];
    call(m,a,o,s,0x17b000,0x16e6a0);
    instruction(0x17b005);a.gp[3]=a.gp[0];
    instruction(0x17b007);a.gp[1]=logic(none);
    instruction(0x17b00a);compare(a.gp[3],a.gp[1]);
    instruction(0x17b00c);if(!(a.eflags&64u))goto allocated;
    instruction(0x17b00e);a.gp[2]=read(m,a.gp[6]);
    instruction(0x17b010);a.gp[2]=right_two(a.gp[2]);
    instruction(0x17b013);logic(a.gp[2]&1u,1);
    instruction(0x17b016);if(a.eflags&64u)goto checked;
    call(m,a,o,s,0x17b018,0x17aef0);
    instruction(0x17b01d);logic(a.gp[0]&255u,1);
    instruction(0x17b01f);if(a.eflags&64u)goto checked;
    instruction(0x17b021);a.gp[2]=a.gp[5];
    call(m,a,o,s,0x17b023,0x16e6a0);
    instruction(0x17b028);a.gp[3]=a.gp[0];
checked:
    instruction(0x17b02a);a.gp[1]=logic(none);
    instruction(0x17b02d);compare(a.gp[3],a.gp[1]);
    instruction(0x17b02f);if(a.eflags&64u)goto complete;
allocated:
    instruction(0x17b035);a.gp[0]=read(m,a.gp[5]+0x44);
    instruction(0x17b038);a.gp[6]=a.gp[3];
    instruction(0x17b03a);a.gp[6]=logic(a.gp[6]&65535u);
    instruction(0x17b040);a.gp[6]=state_product(r,a.gp[6],0x190u);
    instruction(0x17b046);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[0],4,false);
    instruction(0x17b048);a.gp[0]=read(m,a.gp[4]+0x14);
    instruction(0x17b04c);logic(a.gp[0]);
    instruction(0x17b04e);store(m,o,0x17b04e,a.gp[6]+4,a.gp[7]);
    instruction(0x17b051);
    if(!(a.eflags&64u)) {
        instruction(0x17b053);a.gp[2]=read(m,a.gp[0]);
        instruction(0x17b055);a.gp[1]=a.gp[6]+0x4c;
        instruction(0x17b058);store(m,o,0x17b058,a.gp[1],a.gp[2]);
        instruction(0x17b05a);a.gp[2]=read(m,a.gp[0]+4);
        instruction(0x17b05d);store(m,o,0x17b05d,a.gp[1]+4,a.gp[2]);
        instruction(0x17b060);a.gp[0]=read(m,a.gp[0]+8);
        instruction(0x17b063);store(m,o,0x17b063,a.gp[1]+8,a.gp[0]);
        instruction(0x17b066);
    }else {
        instruction(0x17b068);store(m,o,0x17b068,a.gp[6]+3,logic(read(m,a.gp[6]+3,1)|1u,1),1);
        instruction(0x17b06c);store(m,o,0x17b06c,a.gp[6]+0x50,a.gp[1]);
        instruction(0x17b06f);store(m,o,0x17b06f,a.gp[6]+0x4c,a.gp[1]);
        instruction(0x17b072);store(m,o,0x17b072,a.gp[6]+0x54,a.gp[1],2);
    }
    instruction(0x17b076);a.gp[0]=a.gp[6];
    call(m,a,o,s,0x17b078,0x17b960);
    instruction(0x17b07d);a.xmm[0]={};
    instruction(0x17b080);a.gp[0]=logic(none);
    instruction(0x17b083);a.gp[1]=0xff808080u;
    instruction(0x17b088);store(m,o,0x17b088,a.gp[6]+8,a.gp[0]);
    instruction(0x17b08b);store(m,o,0x17b08b,a.gp[6]+0x48,a.gp[0]);
    instruction(0x17b08e);store(m,o,0x17b08e,a.gp[6]+0x58,a.gp[0]);
    instruction(0x17b091);store(m,o,0x17b091,a.gp[6]+2,0,2);
    instruction(0x17b097);store(m,o,0x17b097,a.gp[6]+0xfc,a.gp[0]);
    instruction(0x17b09d);store(m,o,0x17b09d,a.gp[6]+0x100,a.gp[0]);
    instruction(0x17b0a3);store(m,o,0x17b0a3,a.gp[6]+0x104,a.gp[1]);
    instruction(0x17b0a9);store(m,o,0x17b0a9,a.gp[6]+0x108,a.gp[1]);
    instruction(0x17b0af);store(m,o,0x17b0af,a.gp[6]+0x5e,a.gp[0],2);
    instruction(0x17b0b3);a.gp[7]=logic(0);
    instruction(0x17b0b5);a.gp[2]=a.gp[3];
    instruction(0x17b0b7);store(m,o,0x17b0b7,a.gp[6]+0x74,a.xmm[0][0]);
    instruction(0x17b0bc);store(m,o,0x17b0bc,a.gp[6]+0x78,a.xmm[0][0]);
    call(m,a,o,s,0x17b0c1,0x17b130);
complete:
    instruction(0x17b0c6);a.gp[5]=pop(m,a);
    instruction(0x17b0c7);a.gp[0]=a.gp[3];
    instruction(0x17b0c9);a.gp[3]=pop(m,a);
restore_source:
    instruction(0x17b0ca);a.gp[6]=pop(m,a);
final_return:
    instruction(0x17b0cb);a.gp[7]=pop(m,a);
    instruction(0x17b0cc);(void)pop(m,a);a.gp[4]+=8;
}
// 178E30 owns effect creation from the authored request. Keep admission,
// attachment, placement, component setup, restart and final validation in
// their source order. Each lower owner receives its actual ABI and supplies
// its actual return; this owner never supplies a successful allocation.
void create_effect(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};
    IteratorRegisters r{a.gp,a.eflags};
    const auto logic=[&](std::uint32_t value,unsigned width=4){iterator_logic_flags(r,value,width);return value;};
    const auto compare=[&](std::uint32_t lhs,std::uint32_t rhs){iterator_flags(r,lhs,rhs,4,true);};
    const auto shift=[&](std::uint32_t value,unsigned count,unsigned width=4) {
        const auto mask=width==4?none:255u;value&=mask;
        const auto result=value>>count;iterator_logic_flags(r,result,width);
        a.eflags=(a.eflags&~0x801u)|((value>>(count-1u))&1u)|
            (count==1u && (value&(width==4?0x80000000u:0x80u))?0x800u:0u);
        return result;
    };
    instruction(0x178e30);push(m,a,o,0x178e30,a.gp[1]);
    instruction(0x178e31);push(m,a,o,0x178e31,a.gp[6]);
    instruction(0x178e32);push(m,a,o,0x178e32,a.gp[7]);
    instruction(0x178e33);a.gp[7]=read(m,a.gp[4]+0x10);
    instruction(0x178e37);a.gp[0]=read(m,a.gp[7]);
    instruction(0x178e39);a.gp[0]=shift(a.gp[0],2);
    instruction(0x178e3c);a.gp[6]=logic(none);
    instruction(0x178e3f);a.gp[0]=(a.gp[0]&0xffffff00u)|logic(a.gp[0]&1u,1);
    instruction(0x178e41);store(m,o,0x178e41,a.gp[4]+0x10,a.gp[0],1);
    instruction(0x178e45);
    if(!(a.eflags&64u))goto admitted;
    call(m,a,o,s,0x178e47,0x1790a0);
    instruction(0x178e4c);logic(a.gp[0]&255u,1);
    instruction(0x178e4e);
    if(a.eflags&64u) {
        instruction(0x17908d);a.gp[7]=pop(m,a);
        instruction(0x17908e);a.gp[0]=a.gp[6];
        instruction(0x179090);a.gp[6]=pop(m,a);
        instruction(0x179091);a.gp[1]=pop(m,a);
        instruction(0x179092);(void)pop(m,a);a.gp[4]+=4;return;
    }
admitted:
    instruction(0x178e54);a.gp[1]=read(m,a.gp[4]+0x10);
    instruction(0x178e58);a.gp[0]=read(m,a.gp[7]+4);
    instruction(0x178e5b);push(m,a,o,0x178e5b,a.gp[5]);
    instruction(0x178e5c);push(m,a,o,0x178e5c,a.gp[1]);
    instruction(0x178e5d);a.gp[2]=a.gp[7]+12;
    instruction(0x178e60);push(m,a,o,0x178e60,a.gp[2]);
    call(m,a,o,s,0x178e61,0x17afb0);
    instruction(0x178e66);a.gp[5]=a.gp[0];
    instruction(0x178e68);compare(a.gp[5],none);
    instruction(0x178e6b);store(m,o,0x178e6b,a.gp[4]+0x14,a.gp[5]);
    instruction(0x178e6f);if(a.eflags&64u)goto completed;
    instruction(0x178e75);a.gp[0]=read(m,0x4f720c);
    instruction(0x178e7a);a.gp[1]=read(m,a.gp[0]+0x44);
    instruction(0x178e7d);a.gp[0]=read(m,a.gp[7]+0x30);
    instruction(0x178e80);a.gp[6]=a.gp[5];
    instruction(0x178e82);a.gp[6]=logic(a.gp[6]&65535u);
    instruction(0x178e88);a.gp[6]=state_product(r,a.gp[6],0x190u);
    instruction(0x178e8e);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[1],4,false);
    instruction(0x178e90);a.gp[1]=read(m,a.gp[6]+4);
    instruction(0x178e93);a.gp[1]=logic(a.gp[1]&65535u);
    instruction(0x178e99);a.gp[1]<<=4;iterator_logic_flags(r,a.gp[1],4);a.eflags&=~0x801u;
    instruction(0x178e9c);push(m,a,o,0x178e9c,a.gp[3]);
    instruction(0x178e9d);a.gp[3]=read(m,0x4f0414);
    instruction(0x178ea3);a.gp[2]=read(m,a.gp[1]+a.gp[3]+8);
    instruction(0x178ea7);a.gp[1]=read(m,0x4f3218);
    instruction(0x178ead);store(m,o,0x178ead,a.gp[6]+12,a.gp[0]);
    instruction(0x178eb0);compare(read(m,a.gp[1]+8),2);
    instruction(0x178eb4);store(m,o,0x178eb4,a.gp[4]+0x10,a.gp[2]);
    instruction(0x178eb8);
    if(a.eflags&64u) {
        call(m,a,o,s,0x178eba,0x165b00);
        instruction(0x178ebf);logic(a.gp[0]&255u,1);
        instruction(0x178ec1);
        if(!(a.eflags&64u)) {
            instruction(0x178ec3);a.gp[0]=read(m,a.gp[7]+8);
            instruction(0x178ec6);compare(a.gp[0],none);
            instruction(0x178ec9);
            if(!(a.eflags&64u)) {
                instruction(0x178ecb);a.gp[0]=logic(a.gp[0]&65535u);
                instruction(0x178ed0);a.gp[2]=a.gp[0]*3u;
                instruction(0x178ed3);a.gp[0]=read(m,0x4ecbd0);
                instruction(0x178ed8);a.gp[1]=read(m,a.gp[0]+0x44);
                instruction(0x178edb);a.gp[2]=read(m,a.gp[1]+a.gp[2]*4u+8);
                instruction(0x178edf);a.gp[0]=read(m,a.gp[2]);
                instruction(0x178ee1);a.gp[0]=logic(a.gp[0]&65535u);
                instruction(0x178ee6);a.gp[0]<<=4;iterator_logic_flags(r,a.gp[0],4);a.eflags&=~0x801u;
                instruction(0x178ee9);a.gp[1]=read(m,a.gp[0]+a.gp[3]+8);
                instruction(0x178eed);a.gp[2]=logic(0);
                instruction(0x178eef);a.gp[2]=read(m,a.gp[1]+2,2);
                instruction(0x178ef3);a.gp[2]=shift(a.gp[2],13);
                instruction(0x178ef6);logic(a.gp[2]&1u,1);
                instruction(0x178ef9);
                if(!(a.eflags&64u)) {
                    instruction(0x178efb);store(m,o,0x178efb,a.gp[6]+3,logic(read(m,a.gp[6]+3,1)|4u,1),1);
                    instruction(0x178eff);
                }else {
                    instruction(0x178f01);store(m,o,0x178f01,a.gp[6]+3,logic(read(m,a.gp[6]+3,1)&0xfbu,1),1);
                }
            }
        }
    }
    instruction(0x178f05);logic(read(m,a.gp[7],1)&1u,1);
    instruction(0x178f08);
    if(!(a.eflags&64u)) {
        instruction(0x178f0a);a.gp[3]=read(m,a.gp[7]+8);
        instruction(0x178f0d);push(m,a,o,0x178f0d,a.gp[3]);
        call(m,a,o,s,0x178f0e,0x169112);
        instruction(0x178f13);a.gp[5]=a.gp[0];
        instruction(0x178f15);push(m,a,o,0x178f15,none);
        instruction(0x178f17);a.gp[0]=a.gp[3];
        call(m,a,o,s,0x178f19,0xbcdf0);
        instruction(0x178f1e);logic(a.gp[0]);
        instruction(0x178f20);
        if(!(a.eflags&64u)){instruction(0x178f22);store(m,o,0x178f22,a.gp[6]+0x48,a.gp[3]);}
        instruction(0x178f25);compare(a.gp[5],none);
        instruction(0x178f28);
        if(!(a.eflags&64u)) {
            instruction(0x178f2a);a.gp[0]=read(m,a.gp[7]+8);
            instruction(0x178f2d);push(m,a,o,0x178f2d,a.gp[0]);
            instruction(0x178f2e);a.gp[1]=a.gp[5];
            call(m,a,o,s,0x178f30,0x169151);
            instruction(0x178f35);logic(a.gp[0]&255u,1);
            instruction(0x178f37);
            if(!(a.eflags&64u)){instruction(0x178f39);store(m,o,0x178f39,a.gp[6]+0x58,a.gp[5]);}
        }
        instruction(0x178f3c);store(m,o,0x178f3c,a.gp[6]+2,logic(read(m,a.gp[6]+2,1)|0x80u,1),1);
        instruction(0x178f40);a.gp[5]=read(m,a.gp[4]+0x18);
        instruction(0x178f44);
    }else {
        instruction(0x178f46);store(m,o,0x178f46,a.gp[6]+0x48,none);
        instruction(0x178f4d);a.gp[1]=a.gp[7]+0x24;
        instruction(0x178f50);a.gp[0]=read(m,a.gp[1]);
        instruction(0x178f52);a.gp[2]=a.gp[6]+0x30;
        instruction(0x178f55);store(m,o,0x178f55,a.gp[2],a.gp[0]);
        instruction(0x178f57);a.gp[0]=read(m,a.gp[1]+4);
        instruction(0x178f5a);store(m,o,0x178f5a,a.gp[2]+4,a.gp[0]);
        instruction(0x178f5d);a.gp[1]=read(m,a.gp[1]+8);
        instruction(0x178f60);store(m,o,0x178f60,a.gp[2]+8,a.gp[1]);
    }
    instruction(0x178f63);a.gp[2]=read(m,a.gp[7]);
    instruction(0x178f65);a.gp[2]=shift(a.gp[2],1);
    instruction(0x178f67);logic(a.gp[2]&1u,1);
    instruction(0x178f6a);
    if(!(a.eflags&64u)) {
        instruction(0x178f6c);store(m,o,0x178f6c,a.gp[6]+2,logic(read(m,a.gp[6]+2,1)|2u,1),1);
        instruction(0x178f70);a.gp[0]=read(m,a.gp[7]+0x34);
        instruction(0x178f73);store(m,o,0x178f73,a.gp[6]+0x10,a.gp[0]);
        instruction(0x178f76);a.gp[1]=read(m,a.gp[7]+0x38);
        instruction(0x178f79);store(m,o,0x178f79,a.gp[6]+0x14,a.gp[1]);
        instruction(0x178f7c);a.gp[2]=(a.gp[2]&0xffff0000u)|read(m,a.gp[7]+0x3c,2);
        instruction(0x178f80);a.gp[0]=logic(0);
        instruction(0x178f82);store(m,o,0x178f82,a.gp[6]+0x18,a.gp[2],2);
        instruction(0x178f86);store(m,o,0x178f86,a.gp[6]+0x40,a.gp[0]);
        instruction(0x178f89);store(m,o,0x178f89,a.gp[6]+0x44,a.gp[0]);
        instruction(0x178f8c);
    }else {
        instruction(0x178f8e);a.gp[0]=read(m,a.gp[7]+0x44);
        instruction(0x178f91);a.gp[2]=read(m,a.gp[7]+0x4c);
        instruction(0x178f94);a.gp[1]=read(m,a.gp[7]+0x48);
        instruction(0x178f97);scalar(m,a,0,a.gp[7]+0x40);
        instruction(0x178f9c);push(m,a,o,0x178f9c,a.gp[0]);
        instruction(0x178f9d);a.gp[0]=a.gp[6];
        call(m,a,o,s,0x178f9f,0x17b0d0);
    }
    instruction(0x178fa4);a.gp[0]=read(m,a.gp[6]+0x48);
    instruction(0x178fa7);compare(a.gp[0],none);
    instruction(0x178faa);
    if(!(a.eflags&64u)) {
        instruction(0x178fac);a.gp[1]=(a.gp[1]&0xffffff00u)|read(m,0x51df7c,1);
        instruction(0x178fb2);logic(a.gp[1]&255u,1);
        instruction(0x178fb4);
        if(!(a.eflags&64u)) {
            call(m,a,o,s,0x178fb6,0x178de0);
            instruction(0x178fbb);logic(a.gp[0]&255u,1);
            instruction(0x178fbd);
            if(!(a.eflags&64u)){instruction(0x178fbf);store(m,o,0x178fbf,a.gp[6]+2,logic(read(m,a.gp[6]+2,1)|0x20u,1),1);}
        }
    }
    instruction(0x178fc3);a.gp[1]=a.gp[6]+0x7c;
    instruction(0x178fc6);a.gp[0]=logic(none);
    call(m,a,o,s,0x178fc9,0x17e5e0);
    instruction(0x178fce);a.gp[2]=read(m,a.gp[7]+0x20);
    instruction(0x178fd1);a.gp[0]=read(m,a.gp[7]+0x1c);
    instruction(0x178fd4);a.gp[1]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[7]+0x18,2)))));
    instruction(0x178fd8);push(m,a,o,0x178fd8,a.gp[2]);
    instruction(0x178fd9);a.gp[2]=read(m,a.gp[6]+0x58);
    instruction(0x178fdc);push(m,a,o,0x178fdc,a.gp[0]);
    instruction(0x178fdd);a.gp[0]=read(m,a.gp[6]+0x48);
    instruction(0x178fe0);push(m,a,o,0x178fe0,a.gp[2]);
    instruction(0x178fe1);push(m,a,o,0x178fe1,a.gp[0]);
    instruction(0x178fe2);a.gp[0]=a.gp[5];
    call(m,a,o,s,0x178fe4,0x17b1f0);
    instruction(0x178fe9);logic(read(m,a.gp[7],1)&1u,1);
    instruction(0x178fec);
    if(a.eflags&64u)call(m,a,o,s,0x178fee,0x17b880);
    instruction(0x178ff3);a.gp[3]=read(m,a.gp[4]+0x10);
    instruction(0x178ff7);a.gp[0]=read(m,a.gp[3]);
    instruction(0x178ff9);a.gp[1]=a.gp[0];
    instruction(0x178ffb);a.gp[1]=shift(a.gp[1],3);
    instruction(0x178ffe);logic(a.gp[1]&1u,1);
    instruction(0x179001);
    if(a.eflags&64u) {
        instruction(0x179003);a.gp[0]=shift(a.gp[0],4);
        instruction(0x179006);logic(a.gp[0]&1u,1);
        instruction(0x179008);if(a.eflags&64u)goto placed;
    }
    instruction(0x17900a);a.gp[0]=read(m,a.gp[7]+8);
    instruction(0x17900d);push(m,a,o,0x17900d,1);
    instruction(0x17900f);push(m,a,o,0x17900f,a.gp[7]);
    instruction(0x179010);a.gp[1]=a.gp[6];
    call(m,a,o,s,0x179012,0x17e460);
placed:
    instruction(0x179017);a.gp[0]=read(m,a.gp[7]+0x50);
    instruction(0x17901a);logic(a.gp[0]);
    instruction(0x17901c);
    if(!(a.eflags&64u)) {
        instruction(0x17901e);a.gp[2]=read(m,a.gp[0]);
        instruction(0x179020);store(m,o,0x179020,a.gp[6]+0x74,a.gp[2]);
        instruction(0x179023);a.gp[0]=read(m,a.gp[0]+4);
        instruction(0x179026);store(m,o,0x179026,a.gp[6]+0x78,a.gp[0]);
    }
    instruction(0x179029);push(m,a,o,0x179029,a.gp[5]);
    call(m,a,o,s,0x17902a,0x17a1a0);
    instruction(0x17902f);a.gp[2]=read(m,a.gp[3]);
    instruction(0x179031);a.gp[1]=read(m,a.gp[7]+0x5c);
    instruction(0x179034);a.gp[2]=shift(a.gp[2],1);
    instruction(0x179036);logic(a.gp[2]&1u,1);
    instruction(0x179039);store(m,o,0x179039,0x51df78,a.gp[1]);
    instruction(0x17903f);a.gp[3]=pop(m,a);
    instruction(0x179040);if(a.eflags&64u)goto zero_blend;
    instruction(0x179042);a.gp[0]=logic(0);
    instruction(0x179044);a.gp[0]=read(m,a.gp[6]+2,2);
    instruction(0x179048);a.gp[1]=(a.gp[1]&0xffffff00u)|(a.gp[0]&255u);
    instruction(0x17904a);a.gp[1]=(a.gp[1]&0xffffff00u)|shift(a.gp[1]&255u,1,1);
    instruction(0x17904c);logic(a.gp[1]&1u,1);
    instruction(0x17904f);if(!(a.eflags&64u))goto zero_blend;
    instruction(0x179051);a.gp[0]=shift(a.gp[0],9);
    instruction(0x179054);logic(a.gp[0]&1u,1);
    instruction(0x179056);if(!(a.eflags&64u))goto zero_blend;
    instruction(0x179058);push(m,a,o,0x179058,0x3f800000u);
    instruction(0x17905d);goto blend;
zero_blend:
    instruction(0x17905f);push(m,a,o,0x17905f,0);
blend:
    instruction(0x179061);a.gp[0]=a.gp[5];
    call(m,a,o,s,0x179063,0x17c6e0);
    instruction(0x179068);a.gp[6]=read(m,0x4f720c);
    instruction(0x17906e);a.gp[0]=a.gp[5];
    instruction(0x179070);store(m,o,0x179070,0x51df78,0);
    call(m,a,o,s,0x17907a,0x16e940);
    instruction(0x17907f);logic(a.gp[0]);
    instruction(0x179081);if(!(a.eflags&64u))goto completed;
    instruction(0x179083);a.gp[5]=pop(m,a);
    instruction(0x179084);a.gp[7]=pop(m,a);
    instruction(0x179085);a.gp[0]=logic(none);
    instruction(0x179088);a.gp[6]=pop(m,a);
    instruction(0x179089);a.gp[1]=pop(m,a);
    instruction(0x17908a);(void)pop(m,a);a.gp[4]+=4;return;
completed:
    instruction(0x179095);a.gp[0]=a.gp[5];
    instruction(0x179097);a.gp[5]=pop(m,a);
    instruction(0x179098);a.gp[7]=pop(m,a);
    instruction(0x179099);a.gp[6]=pop(m,a);
    instruction(0x17909a);a.gp[1]=pop(m,a);
    instruction(0x17909b);(void)pop(m,a);a.gp[4]+=4;
}
void reset_zoom(Memory &m,Abi &a,const Observer &o,const Services &s,
    const NativeWeaponZoomResetObserverTu15 &observe_reset) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    const auto logic=[&](std::uint32_t value,unsigned width=4){iterator_logic_flags(r,value,width);};
    const auto compare=[&](std::uint32_t lhs,std::uint32_t rhs,unsigned width=4){iterator_flags(r,lhs,rhs,width,true);};
    const auto multiply=[&](std::uint32_t value,std::uint32_t factor) {
        const auto product=std::int64_t(std::bit_cast<std::int32_t>(value))*std::int64_t(factor);
        const auto result=std::uint32_t(product);const auto overflow=product!=std::int64_t(std::bit_cast<std::int32_t>(result));
        a.eflags=(a.eflags&~0x801u)|(overflow?0x801u:0u);return result;
    };
    const auto shift=[&](std::uint32_t value,unsigned count) {
        const auto result=value<<count;logic(result);a.eflags=(a.eflags&~0x801u)|((value>>(32u-count))&1u);return result;
    };
    const auto unit=a.gp[0];std::uint32_t player=none;std::uint8_t previous=0xff,request{};bool scoped{};
    instruction(0xcabc0);push(m,a,o,0xcabc0,a.gp[3]);
    instruction(0xcabc1);push(m,a,o,0xcabc1,a.gp[5]);
    instruction(0xcabc2);push(m,a,o,0xcabc2,a.gp[6]);
    instruction(0xcabc3);push(m,a,o,0xcabc3,a.gp[7]);
    instruction(0xcabc4);a.gp[7]=a.gp[0];
    instruction(0xcabc6);a.gp[0]&=65535u;logic(a.gp[0]);
    instruction(0xcabcb);a.gp[6]=a.gp[0]*3u;
    instruction(0xcabce);a.gp[0]=read(m,0x4ecbd0);
    instruction(0xcabd3);a.gp[1]=read(m,a.gp[0]+0x44);
    instruction(0xcabd6);a.gp[6]=shift(a.gp[6],2);
    instruction(0xcabd9);a.gp[5]=read(m,a.gp[1]+a.gp[6]+8);
    instruction(0xcabdd);push(m,a,o,0xcabdd,3);
    instruction(0xcabdf);a.gp[0]=a.gp[7];
    call(m,a,o,s,0xcabe1,0xbcdf0);
    instruction(0xcabe6);logic(a.gp[0]);
    instruction(0xcabe8);if(a.eflags&64u)goto no_player;
    instruction(0xcabea);compare(read(m,a.gp[0]+0x13c),none);
    instruction(0xcabf1);if(!(a.eflags&64u))goto get_player;
no_player:
    instruction(0xcabf3);a.gp[3]=none;logic(a.gp[3]);
    instruction(0xcabf6);goto player_ready;
get_player:
    instruction(0xcabf8);push(m,a,o,0xcabf8,3);
    instruction(0xcabfa);a.gp[0]=a.gp[7];
    call(m,a,o,s,0xcabfc,0xbcdf0);
    instruction(0xcac01);a.gp[1]=none;logic(a.gp[1]);
    instruction(0xcac04);logic(a.gp[0]);
    instruction(0xcac06);
    if(!(a.eflags&64u)){instruction(0xcac08);a.gp[1]=read(m,a.gp[0]+0x13c);}
    instruction(0xcac0e);a.gp[2]=read(m,0x4f54f4);
    instruction(0xcac14);a.gp[0]=read(m,a.gp[2]+0x44);
    instruction(0xcac17);player=a.gp[1];a.gp[1]&=65535u;logic(a.gp[1]);
    instruction(0xcac1d);a.gp[1]=multiply(a.gp[1],0x21c);
    instruction(0xcac23);a.gp[3]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[0]+a.gp[1]+0x28,2)))));
player_ready:
    instruction(0xcac28);a.gp[1]=read(m,0x4ecbd0);
    instruction(0xcac2e);a.gp[2]=read(m,a.gp[1]+0x44);
    instruction(0xcac31);a.gp[6]=read(m,a.gp[2]+a.gp[6]+8);
    instruction(0xcac35);a.gp[0]=(a.gp[0]&0xffff0000u)|std::uint16_t(std::int16_t(std::bit_cast<std::int8_t>(std::uint8_t(read(m,a.gp[6]+0x212,1)))));
    instruction(0xcac3d);a.gp[7]=none;logic(a.gp[7]);
    instruction(0xcac40);compare(a.gp[0],65535u,2);
    instruction(0xcac44);if(a.eflags&64u)goto no_weapon;
    instruction(0xcac46);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[0]))));
    instruction(0xcac49);a.gp[7]=read(m,a.gp[6]+a.gp[0]*4u+0x218);
    instruction(0xcac50);compare(a.gp[7],none);
    instruction(0xcac53);if(!(a.eflags&64u))goto check_scope;
no_weapon:
    instruction(0xcac55);a.gp[0]&=0xffffff00u;logic(0,1);
    instruction(0xcac57);goto scope_ready;
check_scope:
    instruction(0xcac59);a.gp[0]=a.gp[7];
    call(m,a,o,s,0xcac5b,0x106040);
scope_ready:
    instruction(0xcac60);scoped=(a.gp[0]&255u)!=0;a.gp[1]=(a.gp[1]&0xffffff00u)|read(m,a.gp[4]+0x14,1);request=std::uint8_t(a.gp[1]);
    instruction(0xcac64);a.gp[2]&=0xffffff00u;logic(0,1);
    instruction(0xcac66);logic(a.gp[1]&255u,1);
    instruction(0xcac68);if(a.eflags&64u)goto commit_reset;
    instruction(0xcac6a);logic(a.gp[0]&255u,1);
    instruction(0xcac6c);if(!(a.eflags&64u))goto camera_ready;
commit_reset:
    instruction(0xcac6e);a.gp[2]=(a.gp[2]&0xffffff00u)|read(m,a.gp[5]+0x240,1);previous=std::uint8_t(a.gp[2]);
    instruction(0xcac74);a.xmm[0]={};
    instruction(0xcac77);compare(a.gp[2],255u,1);
    instruction(0xcac7a);a.gp[2]=(a.gp[2]&0xffffff00u)|((a.eflags&64u)?0u:1u);
    instruction(0xcac7d);compare(a.gp[3],none);
    instruction(0xcac80);store(m,o,0xcac80,a.gp[5]+0x240,255,1);
    instruction(0xcac87);store(m,o,0xcac87,a.gp[5]+0x241,255,1);
    instruction(0xcac8e);store(m,o,0xcac8e,a.gp[5]+0x26c,a.xmm[0][0]);
    instruction(0xcac96);
    if(observe_reset && previous!=0xffu)observe_reset({unit,player,a.gp[7],previous,request,scoped,
        std::bit_cast<std::int16_t>(std::uint16_t(a.gp[3]))});
    if(a.eflags&64u)goto done;
    instruction(0xcac9c);a.gp[6]=read(m,0x4f9b54);
    instruction(0xcaca2);a.gp[1]=a.gp[3];
    instruction(0xcaca4);a.gp[1]=multiply(a.gp[1],0x94);
    instruction(0xcacaa);a.gp[1]+=a.gp[6]+0x14u;
    instruction(0xcacae);store(m,o,0xcacae,a.gp[1]+0x2e,65535,2);
    instruction(0xcacb4);store(m,o,0xcacb4,a.gp[1]+0x89,0,1);
camera_ready:
    instruction(0xcacbb);compare(a.gp[3],none);
    instruction(0xcacbe);if(a.eflags&64u)goto done;
    instruction(0xcacc0);logic(a.gp[2]&255u,1);
    instruction(0xcacc2);if(a.eflags&64u)goto done;
    instruction(0xcacc4);logic(a.gp[0]&255u,1);
    instruction(0xcacc6);if(a.eflags&64u)goto default_sound;
    instruction(0xcacc8);a.gp[0]=(a.gp[0]&0xffffff00u)|read(m,a.gp[4]+0x14,1);
    instruction(0xcaccc);logic(a.gp[0]&255u,1);
    instruction(0xcacce);if(!(a.eflags&64u))goto default_sound;
    instruction(0xcacd0);compare(a.gp[7],none);
    instruction(0xcacd3);if(a.eflags&64u)goto done;
    instruction(0xcacd5);a.gp[0]=read(m,0x4ecbd0);
    instruction(0xcacda);a.gp[1]=read(m,a.gp[0]+0x44);
    instruction(0xcacdd);a.gp[7]&=65535u;logic(a.gp[7]);
    instruction(0xcace3);a.gp[2]=a.gp[7]*3u;
    instruction(0xcace6);a.gp[2]=read(m,a.gp[1]+a.gp[2]*4u+8);
    instruction(0xcacea);a.gp[0]=read(m,a.gp[2]);
    instruction(0xcacec);a.gp[1]=read(m,0x4f0414);
    instruction(0xcacf2);a.gp[0]&=65535u;logic(a.gp[0]);
    instruction(0xcacf7);a.gp[0]=shift(a.gp[0],4);
    instruction(0xcacfa);a.gp[2]=read(m,a.gp[0]+a.gp[1]+8);
    instruction(0xcacfe);a.gp[0]=read(m,a.gp[2]+0x278);
    instruction(0xcad04);goto sound_ready;
default_sound:
    instruction(0xcad06);a.gp[0]=read(m,0x4ecc1c);
    instruction(0xcad0b);a.gp[1]=read(m,a.gp[0]+0x134);
    instruction(0xcad11);a.gp[0]=read(m,a.gp[1]+0xd0);
sound_ready:
    instruction(0xcad17);compare(a.gp[0],none);
    instruction(0xcad1a);if(a.eflags&64u)goto done;
    instruction(0xcad1c);a.gp[7]=pop(m,a);
    instruction(0xcad1d);a.gp[6]=pop(m,a);
    instruction(0xcad1e);a.gp[5]=pop(m,a);
    instruction(0xcad1f);a.gp[3]=pop(m,a);
    instruction(0xcad20);store(m,o,0xcad20,a.gp[4]+4,a.gp[0]);
    instruction(0xcad24);scalar(m,a,0,0x46a0d0);
    instruction(0xcad2c);
    if(!s.required_owner)throw std::runtime_error("Native zoom reset requires its actual sound owner");
    s.required_owner(m,{0xcad2c,0x18c4b0,true},a,o);return;
done:
    instruction(0xcad31);a.gp[7]=pop(m,a);
    instruction(0xcad32);a.gp[6]=pop(m,a);
    instruction(0xcad33);a.gp[5]=pop(m,a);
    instruction(0xcad34);a.gp[3]=pop(m,a);
    instruction(0xcad35);(void)pop(m,a);a.gp[4]+=4;
}
// 17DD50/17DDC0 retain the original matrix source selection and thirteen
// individual MOVSD transfers. Each transfer reads before writing, and commits
// its cursors only after a successful store, including backward/aliased copies.
void copy_effect_location(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17dd50);iterator_flags(r,a.gp[0],65535,2,true);
    instruction(0x17dd54);push(m,a,o,0x17dd54,a.gp[6]);
    instruction(0x17dd55);push(m,a,o,0x17dd55,a.gp[7]);
    instruction(0x17dd56);a.gp[7]=a.gp[1];
    instruction(0x17dd58);a.gp[6]=a.gp[2];
    instruction(0x17dd5a);if(a.eflags&64u)goto no_node;
    instruction(0x17dd5c);iterator_logic_flags(r,(a.gp[0]>>8u)&255u,1);
    instruction(0x17dd5e);if(!(a.eflags&128u))goto object_node;
    instruction(0x17dd60);a.gp[1]=read(m,a.gp[6]+0x58);
    instruction(0x17dd63);iterator_flags(r,a.gp[1],none,4,true);
    instruction(0x17dd66);if(a.eflags&64u)goto object_node;
    instruction(0x17dd68);a.gp[0]&=0x7fffu;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17dd6d);push(m,a,o,0x17dd6d,a.gp[7]);
    instruction(0x17dd6e);push(m,a,o,0x17dd6e,a.gp[0]);
    instruction(0x17dd6f);a.gp[0]=read(m,a.gp[6]+0x48);
    instruction(0x17dd72);push(m,a,o,0x17dd72,a.gp[0]);
    call(m,a,o,s,0x17dd73,0x1693ab);
    instruction(0x17dd78);a.gp[7]=pop(m,a);
    instruction(0x17dd79);a.gp[6]=pop(m,a);
    instruction(0x17dd7a);(void)pop(m,a);return;
no_node:
    instruction(0x17dd7b);a.gp[2]=none;iterator_logic_flags(r,a.gp[2],4);
    instruction(0x17dd7e);goto matrix_ready;
object_node:
    instruction(0x17dd80);a.gp[0]&=0x7fffu;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17dd85);a.gp[2]=a.gp[0];
matrix_ready:
    instruction(0x17dd87);a.gp[0]=read(m,a.gp[6]+0x48);
    instruction(0x17dd8a);a.gp[0]&=65535u;iterator_logic_flags(r,a.gp[0],4);
    instruction(0x17dd8f);a.gp[1]=a.gp[0]*3u;
    instruction(0x17dd92);a.gp[0]=read(m,0x4ecbd0);
    instruction(0x17dd97);a.gp[0]=read(m,a.gp[0]+0x44);
    instruction(0x17dd9a);a.gp[1]=read(m,a.gp[0]+a.gp[1]*4u+8);
    instruction(0x17dd9e);a.gp[0]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(read(m,a.gp[1]+0x116,2)))));
    instruction(0x17dda5);a.gp[6]=std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(a.gp[2]))));
    instruction(0x17dda8);a.gp[6]*=0x34u;a.eflags&=~0x801u; // Signed16 times52 cannot overflow signed32.
    instruction(0x17ddab);a.gp[0]=iterator_flags(r,a.gp[0],a.gp[1],4,false);
    instruction(0x17ddad);a.gp[6]=iterator_flags(r,a.gp[6],a.gp[0],4,false);
    instruction(0x17ddaf);a.gp[1]=13;
    do {
        instruction(0x17ddb4);const auto value=read(m,a.gp[6]);store(m,o,0x17ddb4,a.gp[7],value);
        const auto delta=(a.eflags&1024u)?std::uint32_t(-4):4u;a.gp[6]+=delta;a.gp[7]+=delta;--a.gp[1];
    }while(a.gp[1]);
    instruction(0x17ddb6);a.gp[7]=pop(m,a);
    instruction(0x17ddb7);a.gp[6]=pop(m,a);
    instruction(0x17ddb8);(void)pop(m,a);
}
void copy_effect_location_selected(Memory &m,Abi &a,const Observer &o,const Services &s) {
    const auto instruction=[&](auto pc){if(s.instruction)s.instruction(pc);};IteratorRegisters r{a.gp,a.eflags};
    instruction(0x17ddc0);iterator_logic_flags(r,a.gp[0]&255u,1);
    instruction(0x17ddc2);push(m,a,o,0x17ddc2,a.gp[6]);
    instruction(0x17ddc3);a.gp[6]=read(m,a.gp[4]+8);
    instruction(0x17ddc7);push(m,a,o,0x17ddc7,a.gp[7]);
    instruction(0x17ddc8);a.gp[7]=a.gp[1];
    instruction(0x17ddca);if(a.eflags&64u)goto fallback;
    instruction(0x17ddcc);iterator_flags(r,a.gp[6],65535,2,true);
    instruction(0x17ddd0);if(a.eflags&64u)goto fallback;
    instruction(0x17ddd2);iterator_logic_flags(r,a.gp[6]&0x8000u,4);
    instruction(0x17ddd8);if(a.eflags&64u)goto fallback;
    instruction(0x17ddda);a.gp[0]=read(m,a.gp[2]+0x58);
    instruction(0x17dddd);iterator_flags(r,a.gp[0],none,4,true);
    instruction(0x17dde0);if(a.eflags&64u)goto fallback;
    instruction(0x17dde2);a.gp[1]=a.gp[6];
    instruction(0x17dde4);a.gp[1]&=0x7fffu;iterator_logic_flags(r,a.gp[1],4);
    instruction(0x17ddea);push(m,a,o,0x17ddea,a.gp[1]);
    instruction(0x17ddeb);a.gp[1]=read(m,a.gp[2]+0x48);
    instruction(0x17ddee);push(m,a,o,0x17ddee,a.gp[1]);
    call(m,a,o,s,0x17ddef,0x169376);
    instruction(0x17ddf4);a.gp[1]=13;
    instruction(0x17ddf9);a.gp[6]=a.gp[0];
    do {
        instruction(0x17ddfb);const auto value=read(m,a.gp[6]);store(m,o,0x17ddfb,a.gp[7],value);
        const auto delta=(a.eflags&1024u)?std::uint32_t(-4):4u;a.gp[6]+=delta;a.gp[7]+=delta;--a.gp[1];
    }while(a.gp[1]);
    instruction(0x17ddfd);a.gp[7]=pop(m,a);
    instruction(0x17ddfe);a.gp[6]=pop(m,a);
    instruction(0x17ddff);(void)pop(m,a);a.gp[4]+=4;return;
fallback:
    instruction(0x17de02);a.gp[1]=a.gp[7];
    instruction(0x17de04);a.gp[0]=a.gp[6];
    call(m,a,o,s,0x17de06,0x17dd50);
    instruction(0x17de0b);a.gp[7]=pop(m,a);
    instruction(0x17de0c);a.gp[6]=pop(m,a);
    instruction(0x17de0d);(void)pop(m,a);a.gp[4]+=4;
}
#include "native_weapon_marker_matrix_tu15_internal.hpp"
#include "native_weapon_marker_lookup_tu15_internal.hpp"
#include "native_weapon_marker_math_tu15_internal.hpp"
#include "native_weapon_marker_emission_tu15_internal.hpp"
#include "native_weapon_effect_advance_tu15_internal.hpp"
#include "native_weapon_effect_fp_tu15_internal.hpp"
#include "native_weapon_effect_next_event_tu15_internal.hpp"
#include "native_weapon_effect_random_range_tu15_internal.hpp"
#include "native_weapon_effect_frame_tu15_internal.hpp"
#include "native_weapon_effect_systems_tu15_internal.hpp"
}
void native_weapon_effect_iterator_tu15_run(Memory &m,std::array<std::uint32_t,8> &gp,std::uint32_t &eflags,
    const Services &s,const Observer &o) {
    IteratorRegisters a{gp,eflags};next_effect_location(m,a,o,s);
}
void native_weapon_effect_datum_allocate_tu15_run(Memory &m,std::array<std::uint32_t,8> &gp,std::uint32_t &eflags,
    const Services &s,const Observer &o) {
    IteratorRegisters a{gp,eflags};allocate_effect_datum(m,a,o,s);
}
void native_weapon_effect_integer_tu15_run(Memory &m,std::uint32_t entry,std::array<std::uint32_t,8> &gp,std::uint32_t &eflags,
    const Services &s,const Observer &o) {
    IteratorRegisters a{gp,eflags};
    if(entry==0x16ea60u)find_effect_slot(m,a,o,s);
    else if(entry==0x18be00u)sound_is_local(m,a,o,s);
    else if(entry==0x16e6a0u)allocate_effect_datum(m,a,o,s);
    else if(entry==0x17dce0u)next_effect_location(m,a,o,s);
    else throw std::invalid_argument("Unknown integer-only native weapon effect entry");
}
bool native_weapon_effect_dispatch_tu15_owns_entry(std::uint32_t entry) {
    if(entry==0x179ed0u || entry==0x177830u || entry==0x16ea60u || entry==0x179df0u || entry==0x259b0u || entry==0x17c020u || entry==0x17c140u)return true;
    return entry==0x1e1200u || entry==0x144910u || entry==0x145560u || entry==0x1e11b0u || entry==0xfc510u || entry==0xbacd0u || entry==0xbad60u || entry==0x17dd50u || entry==0x17ddc0u || entry==0x18be00u || entry==0x16e6a0u || entry==0xbbe00u || entry==0x178e30u || entry==0x1791c0u || entry==0x17a1a0u || entry==0x17afb0u || entry==0x17b960u || entry==0x17b0d0u || entry==0x17e5e0u || entry==0x17b130u || entry==0x17dc60u || entry==0x17dce0u || entry==0x106f90u || entry==0x108a70u || entry==0x17dff0u ||
        entry==0x179610u || entry==0x18be50u || entry==0x18c4b0u || entry==0xcabc0u;
}
void native_weapon_effect_dispatch_tu15_run(Memory &m,std::uint32_t entry,Abi &a,
    const Services &s,const Observer &o,const NativeWeaponZoomResetObserverTu15 &observe_reset) {
    if(entry==0x179ed0u)advance_effect_systems(m,a,o,s);
    else if(entry==0x177830u)advance_particle_frame(m,a,o,s);
    else if(entry==0x179df0u)advance_effect_frame(m,a,o,s);
    else if(entry==0x16ea60u)native_weapon_effect_integer_tu15_run(m,entry,a.gp,a.eflags,s,o);
    else if(entry==0x259b0u)effect_random_range(m,a,o,s);
    else if(entry==0x17c020u)next_effect_event(m,a,o,s);
    else if(entry==0x17c140u)advance_effect_event(m,a,o,s);
    else if(entry==0x1e1200u)marker_emission(m,a,o,s);
    else if(entry==0x144910u)marker_quaternion_matrix(m,a,o,s);
    else if(entry==0x145560u)marker_transform_product(m,a,o,s);
    else if(entry==0x1e11b0u)marker_lookup(m,a,o,s);
    else if(entry==0xfc510u)marker_parent_mode(m,a,s);
    else if(entry==0xbacd0u)marker_parent_unit(m,a,o,s);
    else if(entry==0xbad60u)marker_matrix_locations(m,a,o,s);
    else if(entry==0x17dd50u)copy_effect_location(m,a,o,s);
    else if(entry==0x17ddc0u)copy_effect_location_selected(m,a,o,s);
    else if(entry==0x18be00u)native_weapon_effect_integer_tu15_run(m,entry,a.gp,a.eflags,s,o);
    else if(entry==0x16e6a0u)native_weapon_effect_datum_allocate_tu15_run(m,a.gp,a.eflags,s,o);
    else if(entry==0xbbe00u)effect_origin(m,a,o,s);
    else if(entry==0x178e30u)create_effect(m,a,o,s);
    else if(entry==0x1791c0u)direction_packets(m,a,o,s);
    else if(entry==0x17a1a0u)restart_effect(m,a,o,s);
    else if(entry==0x17afb0u)allocate_effect(m,a,o,s);
    else if(entry==0x17b960u)initialize_effect_locations(m,a,o,s);
    else if(entry==0x17b0d0u)initialize_effect_placement(m,a,o,s);
    else if(entry==0x17e5e0u)initialize_effect_handles(m,a,o,s);
    else if(entry==0x17b130u)reset_effect_event(m,a,o,s);
    else if(entry==0x17dc60u)reset_effect_components(m,a,o,s);
    else if(entry==0x17dce0u)native_weapon_effect_iterator_tu15_run(m,a.gp,a.eflags,s,o);
    else if(entry==0x106f90u)select_object(m,a,o,s);
    else if(entry==0x108a70u)dispatch(m,a,o,s);
    else if(entry==0x17dff0u)refresh_effect(m,a,o,s);
    else if(entry==0x179610u)create_attached_effect(m,a,o,s);
    else if(entry==0x18be50u)direct_sound(m,a,o,s);
    else if(entry==0x18c4b0u)unowned_zoom_sound(m,a,o,s);
    else if(entry==0xcabc0u)reset_zoom(m,a,o,s,observe_reset);
    else throw std::invalid_argument("Unknown native weapon effect entry");
}
}
