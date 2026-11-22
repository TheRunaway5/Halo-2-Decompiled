#pragma once
#include "native_campaign_ai_goal_port_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"

namespace halo2 {
// TU1.5 1FB770: EAX actor, EDX direction, ESI origin, stack scalar and
// duration. Publish the direction, its dot product with the origin, the
// scalar, duration and activation bytes in retained order. RET 8.
inline void native_campaign_ai_publish_probe_direction_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    using goal_tu15::u32;
    enum Register { eax,ecx,edx,ebx,esp,ebp,esi,edi };
    const goal_tu15::Simd scalar(fp);
    const goal_tu15::Port source{memory,observe};
    auto &gp=abi.gp;
    const auto store=[&](u32 pc,u32 address,u32 value,unsigned width,bool push=false) {
        const bool watched=native_tu15_observes_retained_store(memory,observe,pc,address,width);
        std::array<std::byte,4> bytes;
        for(unsigned i=0;i<width;++i)bytes[i]=std::byte(value>>(8u*i));
        const auto written=std::span<const std::byte>(bytes).first(width);
        SourcePhysicsContactRecordStore event;
        if(watched) {
            event={pc,address,std::vector<std::byte>(width),{written.begin(),written.end()}};
            memory.read(address,event.before);
        }
        memory.write(address,written);
        if(push)gp[esp]=address;
        if(watched)observe(event);
    };
    const auto publish_fp=[&] {
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
        abi.x87_status=fp.status;
    };
    const auto set_flags=[&](u32 low, std::uint8_t overflow) {
        abi.eflags=(abi.eflags&~0x8d5u)|(low&0xd5u)|(u32(overflow)<<11u);
    };

    gp[ecx]=source.r32(0x5020c8u);
    auto actor=gp[eax];
    u32 low;
    std::uint8_t overflow;
    // Keep the flag producer adjacent to IMUL, whose undefined arithmetic
    // flags can retain that producer's values on the physical reference CPU.
    asm volatile("andl $0xffff,%[actor]\n\timull $0x888,%[actor],%[actor]\n\tlahf\n\tseto %[of]\n\tmovzbl %%ah,%%eax"
        :[actor]"+r"(actor),"=&a"(low),[of]"=&q"(overflow)::"cc");
    gp[eax]=actor;
    set_flags(low,overflow);
    store(0x1fb781u,gp[esp]-4u,gp[ebx],4,true);
    gp[ebx]=source.r32(gp[edx]);
    store(0x1fb784u,gp[esp]-4u,gp[edi],4,true);
    const auto actor_rows=source.r32(gp[ecx]+0x44u);
    asm volatile("addl %[rows],%[actor]\n\tlahf\n\tseto %[of]\n\tmovzbl %%ah,%%eax"
        :[actor]"+r"(actor),"=&a"(low),[of]"=&q"(overflow):[rows]"r"(actor_rows):"cc");
    gp[eax]=actor;
    set_flags(low,overflow);
    const auto direction=actor+0x608u;
    gp[ecx]=direction;
    gp[edi]=direction;

    store(0x1fb790u,direction,gp[ebx],4);
    gp[ebx]=source.r32(gp[edx]+4u);
    store(0x1fb795u,direction+4u,gp[ebx],4);
    gp[edx]=source.r32(gp[edx]+8u);
    store(0x1fb79bu,direction+8u,gp[edx],4);

    try {
        auto &dot=abi.xmm[0];
        auto &term=abi.xmm[1];
        dot={source.r32(direction+8u),0,0,0};
        dot[0]=scalar.mul(dot[0],source.r32(gp[esi]+8u));
        term={source.r32(direction+4u),0,0,0};
        term[0]=scalar.mul(term[0],source.r32(gp[esi]+4u));
        dot[0]=scalar.add(dot[0],term[0]);
        term={source.r32(gp[esi]),0,0,0};
        term[0]=scalar.mul(term[0],source.r32(direction));
        dot[0]=scalar.add(dot[0],term[0]);
        store(0x1fb7c2u,direction+0xcu,dot[0],4);

        const auto duration=source.r16(gp[esp]+0x10u);
        gp[ecx]=(gp[ecx]&0xffff0000u)|duration;
        dot={source.r32(gp[esp]+0xcu),0,0,0};
        gp[edi]=source.r32(gp[esp]);
        gp[esp]+=4u;
        store(0x1fb7d3u,actor+0x618u,dot[0],4);
        store(0x1fb7dbu,actor+0x61cu,duration,2);
        store(0x1fb7e2u,actor+0x605u,1,1);
        store(0x1fb7e9u,actor+0x5d8u,0,1);
        gp[eax]=(gp[eax]&0xffffff00u)|1u;
        gp[ebx]=source.r32(gp[esp]);
        gp[esp]+=4u;
        (void)source.r32(gp[esp]);
        gp[esp]+=12u;
    } catch(...) {
        publish_fp();
        throw;
    }
    publish_fp();
#else
    (void)memory;
    (void)abi;
    (void)fp;
    (void)observe;
    throw std::runtime_error("Native 1FB770 requires the x86 scalar FP owner");
#endif
}
} // namespace halo2
