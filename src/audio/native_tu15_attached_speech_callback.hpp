#pragma once

#include "halo2/native_tu15_attached_speech_data.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_tu15_attached_speech_detail {
using namespace physics_world_runtime_detail;

inline long double load_angle(std::uint32_t raw) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double value;
    asm volatile("flds %1" : "=t"(value) : "m"(raw));
    return value;
#else
    throw std::runtime_error("Original attached speech angle backend unavailable");
#endif
}
inline void turn_angle(WorldState &state,bool cosine) {
    auto &angle=state.fp.st(0);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    long double result;
    std::uint16_t status;
    if(cosine)
        asm volatile("fcos\n\tfnstsw %1" : "=t"(result),"=m"(status) : "0"(angle));
    else
        asm volatile("fsin\n\tfnstsw %1" : "=t"(result),"=m"(status) : "0"(angle));
    angle=result;
    state.fp.status=(state.fp.status&~0x600u)|(status&0x600u);
#else
    throw std::runtime_error("Original attached speech direction backend unavailable");
#endif
}
inline std::uint32_t direction_scalar(WorldState &state) {
    const auto angle=state.fp.st(0);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    std::uint32_t value;
    std::uint16_t status;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1"
                 : "=m"(value),"=m"(status) : "m"(angle) : "st");
    state.fp.status=(state.fp.status&~0x200u)|(status&0x200u);
    return value;
#else
    throw std::runtime_error("Original attached speech direction spill backend unavailable");
#endif
}

inline void set_angle(WorldState &s) {
    auto &g=s.gp;
    auto &x=s.xmm;
    auto &m=s.memory;
    const auto read=[&](std::uint32_t pc,std::uint32_t address,unsigned width=4) {
        s.instruction(pc);return r(m,address,width);
    };
    const auto write=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        s.instruction(pc);w(m,s.observe,pc,address,width,value);
    };
    const auto lane=[&](std::uint32_t pc,std::uint32_t address) {
        return _mm_castsi128_ps(_mm_cvtsi32_si128(read(pc,address)));
    };
    const auto bits=[&](unsigned i) {
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x[i])));
    };
    const auto store_lane=[&](std::uint32_t pc,std::uint32_t address,unsigned i) {
        write(pc,address,bits(i));
    };
    const auto mul_mem=[&](std::uint32_t pc,unsigned i,std::uint32_t address) {
        x[i]=_mm_mul_ss(x[i],lane(pc,address));
    };
    const auto add_lane=[&](std::uint32_t pc,unsigned lhs,unsigned rhs) {
        s.instruction(pc);x[lhs]=_mm_add_ss(x[lhs],x[rhs]);
    };
    const auto mul_lane=[&](std::uint32_t pc,unsigned lhs,unsigned rhs) {
        s.instruction(pc);x[lhs]=_mm_mul_ss(x[lhs],x[rhs]);
    };
    const auto sub_lane=[&](std::uint32_t pc,unsigned lhs,unsigned rhs) {
        s.instruction(pc);x[lhs]=_mm_sub_ss(x[lhs],x[rhs]);
    };

    // Preserve the read-modify-write OR's original ordering and byte extent.
    s.instruction(0x18f0b7);
    w(m,s.observe,0x18f0b7,g[6],1,s.flags.logic(r(m,g[6],1)|1u,1));
    g[2]=read(0x18f0ba,native_tu15_attached_speech_origin_address);
    g[7]=read(0x18f0c0,g[2]);
    s.instruction(0x18f0c2);g[0]=g[6]+0x1c;
    write(0x18f0c5,g[0],g[7]);
    g[7]=read(0x18f0c7,g[2]+4);
    write(0x18f0ca,g[0]+4,g[7]);
    g[2]=read(0x18f0cd,g[2]+8);
    write(0x18f0d0,g[0]+8,g[2]);

    g[2]=read(0x18f0d3,0x474d08);
    g[7]=read(0x18f0d9,g[2]);
    s.instruction(0x18f0db);g[0]=g[6]+0xc;
    s.instruction(0x18f0de);g[6]=g[0];
    write(0x18f0e0,g[6],g[7]);
    g[7]=read(0x18f0e2,g[2]+4);
    write(0x18f0e5,g[6]+4,g[7]);
    g[2]=read(0x18f0e8,g[2]+8);
    write(0x18f0eb,g[6]+8,g[2]);

    s.fp.push(load_angle(read(0x18f0ee,g[1]+4)));
    s.instruction(0x18f0f1);turn_angle(s,false);
    x[0]=lane(0x18f0f3,g[0]+4);
    x[2]=lane(0x18f0f8,g[0]);
    x[3]=lane(0x18f0fc,g[0]);
    x[5]=lane(0x18f100,g[0]);
    s.instruction(0x18f104);
    w(m,s.observe,0x18f104,g[4]+0x1c,4,direction_scalar(s));
    s.fp.pop();
    s.fp.push(load_angle(read(0x18f108,g[1]+4)));
    g[1]=read(0x18f10b,0x474d10);
    s.instruction(0x18f111);turn_angle(s,true);
    mul_mem(0x18f113,0,g[1]+8);
    x[1]=lane(0x18f118,g[1]+4);
    mul_mem(0x18f11d,1,g[0]+8);
    mul_mem(0x18f122,2,g[1]+8);
    mul_mem(0x18f127,3,g[1]+4);
    x[4]=lane(0x18f12c,g[4]+0x1c);
    sub_lane(0x18f132,1,0);
    x[0]=lane(0x18f136,g[1]);
    mul_mem(0x18f13a,0,g[0]+8);
    sub_lane(0x18f13f,2,0);
    x[0]=lane(0x18f143,g[0]+4);
    mul_mem(0x18f148,0,g[1]);
    sub_lane(0x18f14c,0,3);
    mul_lane(0x18f150,1,4);
    mul_lane(0x18f154,2,4);
    mul_lane(0x18f158,0,4);
    s.instruction(0x18f15c);
    w(m,s.observe,0x18f15c,g[4]+0x10,4,direction_scalar(s));
    s.fp.pop();
    x[3]=lane(0x18f160,g[4]+0x10);
    mul_lane(0x18f166,5,3);
    add_lane(0x18f16a,5,1);
    x[1]=lane(0x18f16e,g[0]+4);
    mul_lane(0x18f173,1,3);
    mul_mem(0x18f177,3,g[0]+8);
    add_lane(0x18f17c,1,2);
    add_lane(0x18f180,0,3);
    store_lane(0x18f184,g[0],5);
    store_lane(0x18f188,g[0]+4,1);
    store_lane(0x18f18d,g[0]+8,0);
    s.instruction(0x18f192);s.set(0,1,1);
}

inline void configure_direction(WorldState &s) {
    auto &g=s.gp;
    auto &m=s.memory;
    s.instruction(0x18f074);g[1]=r(m,0x4f0414);
    s.instruction(0x18f07a);g[7]=s.flags.logic(g[7]&0xffffu,4);
    s.instruction(0x18f080);g[7]=s.flags.shift(g[7],4,4);
    s.instruction(0x18f083);g[2]=r(m,g[7]+g[1]+8);
    s.instruction(0x18f087);g[1]=std::uint32_t(std::int32_t(
        std::bit_cast<std::int16_t>(std::uint16_t(r(m,g[2]+6,2)))));
    s.instruction(0x18f08b);g[2]=r(m,0x52d664);
    s.instruction(0x18f091);g[1]=s.flags.product(g[1],0x38);
    s.instruction(0x18f094);g[1]=s.flags.plus(g[1],r(m,g[2]+4),4);
    s.instruction(0x18f097);s.set(2,r(m,g[1]+0x28,1),1);
    s.instruction(0x18f09a);g[1]=s.flags.plus(g[1],0x28,4);
    s.instruction(0x18f09d);s.flags.logic(s.reg(2,1)&7u,1);
    s.instruction(0x18f0a0);if(s.flags.zf)return;
    s.instruction(0x18f0a6);s.flags.logic(g[1],4);
    s.instruction(0x18f0a8);if(s.flags.zf)return;
    s.instruction(0x18f0ae);s.flags.logic(s.reg(2,1)&1u,1);
    s.instruction(0x18f0b1);if(!s.flags.zf)set_angle(s);
}

inline void build_attached_location(WorldState &s) {
    auto &g=s.gp;
    s.instruction(0x18f040);s.push(0x18f040,g[3]);
    s.instruction(0x18f041);g[3]=r(s.memory,g[4]+8);
    s.instruction(0x18f045);s.flags.minus(g[3],0xffffffffu,4);
    const bool attached=!s.flags.zf;
    s.instruction(0x18f048);s.push(0x18f048,g[6]);
    s.instruction(0x18f049);g[6]=r(s.memory,g[4]+0x18);
    s.instruction(0x18f04d);s.push(0x18f04d,g[7]);
    s.instruction(0x18f04e);g[7]=r(s.memory,g[4]+0x14);
    s.instruction(0x18f052);
    bool ready=true;
    if(attached) {
        s.instruction(0x18f054);g[0]=r(s.memory,g[4]+0x18);
        s.instruction(0x18f058);s.push(0x18f058,g[6]);
        s.instruction(0x18f059);s.push(0x18f059,g[0]);
        s.instruction(0x18f05a);s.push(0x18f05a,g[7]);
        s.instruction(0x18f05b);s.push(0x18f05b,g[3]);
        s.instruction(0x18f05c);s.push(0x18f05c,0x18f061);
        s.required(0x18f05c,0x18f1a0,false);
        s.instruction(0x18f061);s.flags.logic(s.reg(0,1),1);
        s.instruction(0x18f063);ready=!s.flags.zf;
    }
    if(ready){s.instruction(0x18f065);s.set(0,1,1);}
    s.instruction(0x18f067);s.flags.minus(g[3],0xffffffffu,4);
    s.instruction(0x18f06a);
    if(s.flags.zf)configure_direction(s);
    else {
        s.instruction(0x18f06c);s.flags.logic(s.reg(0,1),1);
        s.instruction(0x18f06e);if(!s.flags.zf)configure_direction(s);
    }
    s.instruction(0x18f194);g[7]=s.pop();
    s.instruction(0x18f195);g[6]=s.pop();
    s.instruction(0x18f196);g[3]=s.pop();
    s.instruction(0x18f197);(void)s.pop();g[4]+=0x10;
}
} // namespace halo2::native_tu15_attached_speech_detail

namespace halo2 {
inline void native_tu15_attached_speech_callback(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
    using namespace native_tu15_attached_speech_detail;
    FpScope scope(fp);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);
    try {build_attached_location(state);}
    catch(...){abi=state.save();throw;}
    abi=state.save();
}
} // namespace halo2
