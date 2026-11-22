#pragma once
#include "native_campaign_ai_goal_port_tu15.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"

namespace halo2 {
// 1FB800: ECX actor datum, EDX output direction, AL result, RET.
inline void native_campaign_ai_update_probe_direction_tu15(SourcePhysicsContactMemory &memory,
    SourcePhysicsWorldRuntimeAbi &abi,NativeTu15PhysicsFrameFp &fp,
    const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_GOAL_EVALUATION_X86
    using goal_tu15::u32;
    enum Register { eax,ecx,edx,ebx,esp,ebp,esi,edi };
    const goal_tu15::Port source{memory,observe};
    const goal_tu15::Simd scalar(fp);
    auto &gp=abi.gp;
    const auto flags=[&](u32 low,u32 overflow=0) {
        abi.eflags=(abi.eflags&~0x8d5u)|(low&0xd5u)|(overflow<<11u);
    };
    const auto test_zero=[&](u32 value,unsigned width) {
        const auto sign=(value>>(width-1u))&1u;
        const auto parity=(std::popcount(value&255u)&1u)==0;
        flags((sign<<7u)|(u32(!value)<<6u)|(u32(parity)<<2u));
    };
    const auto store=[&](u32 pc,u32 address,u32 value,unsigned width,bool push=false) {
        const bool watched=native_tu15_observes_retained_store(memory,observe,pc,address,width);
        std::array<std::byte,4> raw;
        for(unsigned n=0;n<width;++n)raw[n]=std::byte(value>>(8u*n));
        const auto written=std::span<const std::byte>(raw).first(width);
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
    const auto pop_and_return=[&] {
        gp[esi]=source.r32(gp[esp]);
        gp[esp]+=4u;
        (void)source.r32(gp[esp]);
        gp[esp]+=4u;
    };
    const auto result=[&](bool active){gp[eax]=(gp[eax]&0xffffff00u)|u32(active);};
    const auto above=[&](u32 a,u32 b) {
        const auto lhs=std::bit_cast<float>(a),rhs=std::bit_cast<float>(b);
        u32 host{},csr=fp.mxcsr,low{};
        asm volatile("stmxcsr %[h]\n\tldmxcsr %[c]\n\tcomiss %[r],%[l]\n\tlahf\n\tmovzbl %%ah,%%eax\n\tstmxcsr %[c]\n\tldmxcsr %[h]"
            :[h]"+m"(host),[c]"+m"(csr),"=&a"(low):[l]"x"(lhs),[r]"x"(rhs):"cc","memory");
        flags(low);
        fp.mxcsr=csr;
        return (low&0x41u)==0;
    };

    try {
        gp[eax]=source.r32(0x5020c8u);
        auto actor=gp[ecx];
        u32 low;
        std::uint8_t overflow;
        // The physical x86 IMUL can retain flag values from the masked datum.
        asm volatile("andl $0xffff,%[actor]\n\timull $0x888,%[actor],%[actor]\n\tlahf\n\tseto %[of]\n\tmovzbl %%ah,%%eax"
            :[actor]"+r"(actor),"=&a"(low),[of]"=&q"(overflow)::"cc");
        gp[ecx]=actor;
        flags(low,overflow);
        store(0x1fb811u,gp[esp]-4u,gp[esi],4,true);
        const auto rows=source.r32(gp[eax]+0x44u);
        actor+=rows;
        gp[ecx]=actor;
        result(false);
        flags(0x44u);
        const auto enabled=source.r8(actor+0x605u);
        test_zero(enabled,8);
        if(enabled) {
            auto &dot=abi.xmm[0];
            auto &term=abi.xmm[1];
            dot={source.r32(actor+0x610u),0,0,0};
            dot[0]=scalar.mul(dot[0],source.r32(actor+0x240u));
            term={source.r32(actor+0x60cu),0,0,0};
            term[0]=scalar.mul(term[0],source.r32(actor+0x23cu));
            dot[0]=scalar.add(dot[0],term[0]);
            term={source.r32(actor+0x608u),0,0,0};
            term[0]=scalar.mul(term[0],source.r32(actor+0x238u));
            dot[0]=scalar.add(dot[0],term[0]);
            dot[0]=scalar.sub(dot[0],source.r32(actor+0x614u));
            bool clear=above(dot[0],source.r32(actor+0x618u));
            result(true);
            if(!clear) {
                const auto countdown=source.r16(actor+0x61cu);
                test_zero(countdown,16);
                clear=countdown==0;
                if(!clear) {
                    const auto blocked=source.r8(actor+0x5d8u);
                    test_zero(blocked,8);
                    clear=blocked!=0;
                }
            }
            if(clear) {
                result(false);
                flags(0x44u);
                store(0x1fb87fu,actor+0x605u,0,1);
                store(0x1fb885u,actor+0x61cu,0,2);
            } else {
                constexpr std::array sites{0x1fb896u,0x1fb89eu,0x1fb8a7u};
                for(unsigned axis=0;axis<3;++axis) {
                    const auto offset=axis*4u;
                    gp[esi]=source.r32(actor+0x608u+offset);
                    store(sites[axis],gp[edx]+offset,gp[esi],4);
                }
                gp[edx]=0;
                flags(0x44u);
                const auto countdown=source.r16(actor+0x61cu);
                gp[edx]=countdown;
                test_zero(countdown,16);
                if(goal_tu15::sx16(countdown)>0) {
                    --gp[edx];
                    const auto parity=(std::popcount(gp[edx]&255u)&1u)==0;
                    const auto adjust=(countdown&15u)==0;
                    flags((u32(!gp[edx])<<6u)|(u32(adjust)<<4u)|(u32(parity)<<2u));
                    store(0x1fb8b9u,actor+0x61cu,gp[edx],2);
                }
            }
        }
        pop_and_return();
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
    throw std::runtime_error("Native 1FB800 requires the x86 scalar FP owner");
#endif
}
} // namespace halo2
