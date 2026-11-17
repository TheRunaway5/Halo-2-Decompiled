#pragma once
#include "halo2/native_tu15_device_property_query_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
#include <bit>

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_device_property_query_detail {
template<class State,class Before> class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;using Base::push;
    native_raw_x87_detail::Scope<State>& fp_;

    std::uint32_t read(std::uint32_t pc,std::uint32_t address,unsigned width=4) {at(pc);return word(address,width);}
    __m128 scalar(std::uint32_t pc,std::uint32_t address) {return _mm_set_ss(std::bit_cast<float>(read(pc,address)));}
    bool below_or_equal() const {return s_.flags.cf||s_.flags.zf;}
    void lahf_test() {
        s_.set(0,s_.flags.bits()|2u,1,8);
        s_.flags.logic(s_.reg(0,1,8)&0x44u,1);
    }
    std::uint32_t finish(std::uint32_t restore,std::uint32_t ret) {
        s_.gp[3]=pop(restore);pop(ret);s_.gp[4]+=16u;return s_.gp[0];
    }
    std::uint32_t shl4(std::uint32_t value) {
        const auto input=s_.flags.bits();auto flags=((input&0xd5u)<<8u)|((input>>11u)&1u);
        std::uint32_t overflow{};
        // Original 10BE51 is C1 E0 04; preserve the physical flag packet.
        asm volatile("addb $127,%%al;sahf;shll $4,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
            :"+a"(flags),"=c"(overflow),"+r"(value)::"cc");
        s_.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
    }
    void load_extended(std::uint32_t pc,std::uint32_t address) {
        const auto bits=read(pc,address);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void abs_extended() {asm volatile("fabs":::"memory","st");fp_.publish();}
    void dup_extended() {asm volatile("fld %%st(0)":::"memory","st","st(1)");fp_.publish();}
    void pop_extended() {asm volatile("fstp %%st(0)":::"memory","st");fp_.publish();}
    void compare_pop_extended() {
        unsigned char cf{},zf{},pf{};
        asm volatile("fcomip %%st(1),%%st;setb %0;sete %1;setp %2"
            :"=qm"(cf),"=qm"(zf),"=qm"(pf)::"memory","cc","st");
        s_.flags.load(unsigned(cf)|(unsigned(pf)<<2u)|(unsigned(zf)<<6u));fp_.publish();
    }
    void div_extended(std::uint32_t pc,std::uint32_t address) {
        const auto bits=read(pc,address);
        asm volatile("fdivs %0"::"m"(bits):"memory","st");fp_.publish();
    }
    void spill(std::uint32_t pc,std::uint32_t reload) {
        at(pc);const auto pending=fp_.stage_single(true);
        Base::store(pc,s_.gp[4]+8u,pending.bits,[&]{fp_.commit(pending);},true);
        s_.xmm[0]=scalar(reload,s_.gp[4]+8u);
    }
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value) {
        at(pc);SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(s_.observe){event.before.resize(1);Base::io([&]{s_.memory.read(address,event.before);});}
        const std::array data{std::byte(value)};
        Base::io([&]{s_.memory.write(address,data);});
        if(s_.observe){event.written.assign(data.begin(),data.end());Base::io([&]{s_.observe(event);});}
    }
public:
    Frame(State& state,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :Base{state,before,native_mopp_ray_caller_detail::ScopeIo<State>{&fp}},fp_(fp) {}
    Key select() {
        auto& g=s_.gp;
        g[0]=read(0x10be20u,g[4]+4u);g[1]=read(0x10be24u,0x4ecbd0u);
        g[2]=read(0x10be2au,g[1]+0x44u);s_.xmm[1]=_mm_xor_ps(s_.xmm[1],s_.xmm[1]);
        s_.xmm[2]=scalar(0x10be30u,0x46a0d0u);g[0]=s_.flags.logic(g[0]&0xffffu,4);
        g[0]+=g[0]*2u;g[1]=read(0x10be40u,g[2]+g[0]*4u+8u);
        g[0]=read(0x10be44u,g[1]);g[2]=read(0x10be46u,0x4f0414u);
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[0]=shl4(g[0]);
        g[2]=read(0x10be54u,g[0]+g[2]+8u);g[0]=read(0x10be58u,g[4]+8u);
        s_.flags.minus(g[0],0x6000560u,4);
        push(0x10be61u,g[3]);s_.set(3,1u,1,0);s_.xmm[0]=s_.xmm[1];
        if(!s_.flags.zf&&s_.flags.sf==s_.flags.of){
            s_.flags.minus(g[0],0x8000080u,4);if(s_.flags.zf)return Key::scalar_140;
            s_.flags.minus(g[0],0xf00055eu,4);if(s_.flags.zf)return Key::change_power;
            s_.flags.minus(g[0],0x1200055fu,4);return s_.flags.zf?Key::change_position:Key::unknown;
        }
        if(s_.flags.zf)return Key::locked;
        s_.flags.minus(g[0],0x40005abu,4);if(s_.flags.zf)return Key::loop;
        s_.flags.minus(g[0],0x5000081u,4);if(s_.flags.zf)return Key::scalar_134;
        s_.flags.minus(g[0],0x5000561u,4);return s_.flags.zf?Key::delay:Key::unknown;
    }
    std::uint32_t unknown() {s_.set(0,s_.flags.logic(0u,1),1,0);return finish(0x10bf97u,0x10bf98u);}
    Output delay() {
        auto&g=s_.gp;auto*x=s_.xmm;
        x[3]=scalar(0x10be8cu,g[2]+0x10cu);s_.flags.scalar_compare(x[3],x[1],false);
        if(below_or_equal())return Output::direct;
        g[0]=read(0x10be9du,0x51df54u);
        const auto signed_count=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(read(0x10bea2u,g[1]+0x148u,2)));
        g[1]=std::bit_cast<std::uint32_t>(std::int32_t(signed_count));
        x[0]=scalar(0x10bea9u,g[0]+4u);
        asm volatile("divss %1,%0":"+x"(x[0]):"x"(x[3]));
        x[3]=_mm_cvtsi32_ss(x[3],std::bit_cast<std::int32_t>(g[1]));
        asm volatile("mulss %1,%0":"+x"(x[0]):"x"(x[3]));
        return Output::clamped;
    }
    void loop() {
        s_.xmm[0]=scalar(0x10beccu,0x46a194u);
        s_.flags.scalar_compare(s_.xmm[0],scalar(0x10bed4u,s_.gp[1]+0x140u),false);
        s_.gp[0]=s_.flags.cf?s_.flags.logic(0u,4):1u;
        s_.xmm[0]=_mm_cvtsi32_ss(s_.xmm[0],std::bit_cast<std::int32_t>(s_.gp[0]));
    }
    void scalar_134() {s_.xmm[0]=scalar(0x10bebfu,s_.gp[1]+0x134u);}
    void scalar_140() {s_.xmm[0]=scalar(0x10c064u,s_.gp[1]+0x140u);}
    Output locked() {
        auto&g=s_.gp;auto*x=s_.xmm;
        x[3]=scalar(0x10bef6u,g[1]+0x134u);s_.flags.scalar_compare(x[3],x[1],true);
        lahf_test();if(!s_.flags.pf)x[0]=x[2];
        s_.flags.minus(read(0x10bf0au,g[1]+0xaau,1),7u,1);if(!s_.flags.zf)return Output::clamped;
        g[0]=read(0x10bf17u,g[1]+0x13cu);s_.flags.minus(g[0],0xffffffffu,4);
        if(s_.flags.zf)return Output::clamped;
        g[0]=s_.flags.logic(g[0]&0xffffu,4);g[2]=g[0]+g[0]*2u;
        g[0]=read(0x10bf2eu,0x4ecbf8u);g[0]=read(0x10bf33u,g[0]+0x44u);
        g[0]+=g[2]*4u;g[2]=read(0x10bf39u,g[1]+0x1ccu);
        s_.flags.logic(s_.reg(2,1,0)&0xdu,1);if(!s_.flags.zf)x[0]=x[2];
        s_.set(0,read(0x10bf47u,g[0]+2u,2),2,0);
        s_.flags.logic(s_.reg(0,1,0)&1u,1);
        if(!s_.flags.zf){s_.flags.logic(s_.reg(0,1,0)&2u,1);if(!s_.flags.zf)x[0]=x[2];}
        x[3]=scalar(0x10bf56u,g[1]+0x140u);s_.flags.scalar_compare(x[3],x[2],true);lahf_test();
        if(s_.flags.pf){
            s_.flags.logic(s_.reg(2,1,0)&0x10u,1);
            if(s_.flags.zf)return Output::clamped;
        }
        x[0]=x[1];return Output::direct;
    }
    Output change_position() {
        auto&g=s_.gp;auto*x=s_.xmm;
        x[3]=scalar(0x10bf9bu,g[1]+0x144u);s_.flags.scalar_compare(x[3],x[1],true);lahf_test();
        if(!s_.flags.pf)return Output::direct;
        g[0]=read(0x10bfb0u,g[1]+0x12cu);s_.flags.logic(s_.reg(0,1,0)&8u,1);x[0]=x[1];
        if(!s_.flags.zf) {
            s_.flags.logic(s_.reg(0,1,0)&0x20u,1);if(!s_.flags.zf)return Output::direct;
            fp_.require_capacity(3u);
            load_extended(0x10bfc5u,g[1]+0x164u);abs_extended();dup_extended();
            load_extended(0x10bfcfu,0x46a0ecu);compare_pop_extended();pop_extended();
            if(!below_or_equal()){pop_extended();return Output::direct;}
            load_extended(0x10bfdfu,g[1]+0x144u);abs_extended();
            asm volatile("fdiv %%st(1),%%st":::"memory","st");fp_.publish();
            spill(0x10bfe9u,0x10bfedu);pop_extended();
            return Output::clamped;
        }
        fp_.require_capacity(2u);
        load_extended(0x10bff7u,g[2]+0xc8u);abs_extended();
        load_extended(0x10bfffu,0x46a0ecu);compare_pop_extended();pop_extended();
        if(!below_or_equal())return Output::direct;
        load_extended(0x10c00bu,g[1]+0x144u);abs_extended();
        div_extended(0x10c013u,g[2]+0xc8u);spill(0x10c019u,0x10c01du);
        return Output::clamped;
    }
    Output change_power() {
        auto&g=s_.gp;auto*x=s_.xmm;
        x[3]=scalar(0x10c025u,g[1]+0x138u);s_.flags.scalar_compare(x[3],x[1],true);lahf_test();
        if(!s_.flags.pf)return Output::direct;
        x[3]=scalar(0x10c036u,g[2]+0xc0u);s_.flags.scalar_compare(x[3],x[1],true);lahf_test();x[0]=x[1];
        if(!s_.flags.pf)return Output::direct;
        fp_.require_capacity(1u);load_extended(0x10c04au,g[1]+0x138u);abs_extended();
        div_extended(0x10c052u,g[2]+0xc0u);spill(0x10c058u,0x10c05cu);
        return Output::clamped;
    }
    void clamp() {
        auto*x=s_.xmm;
        s_.flags.scalar_compare(x[1],x[0],false);
        if(!below_or_equal()){x[0]=x[1];return;}
        s_.flags.scalar_compare(x[0],x[2],false);
        if(!below_or_equal())x[0]=x[2];
    }
    std::uint32_t publish() {
        auto&g=s_.gp;auto*x=s_.xmm;
        s_.flags.scalar_compare(x[0],x[1],false);
        g[1]=read(0x10c085u,g[4]+16u);
        Base::store(0x10c089u,g[1],std::bit_cast<std::uint32_t>(_mm_cvtss_f32(x[0])),[]{});
        if(below_or_equal()){
            g[2]=read(0x10c0a0u,g[4]+20u);g[0]=s_.flags.logic(0u,4);
            store_byte(0x10c0a6u,g[2],s_.reg(0,1,0));s_.set(0,s_.reg(3,1,0),1,0);
            return finish(0x10c0aau,0x10c0abu);
        }
        g[2]=read(0x10c08fu,g[4]+20u);g[0]=1u;
        store_byte(0x10c098u,g[2],s_.reg(0,1,0));s_.set(0,s_.reg(3,1,0),1,0);
        return finish(0x10c09cu,0x10c09du);
    }
};

template<class State,class Before> std::uint32_t run_private(State& state,Before before) {
    native_raw_x87_detail::Scope fp{state};Frame view{state,before,fp};return query(view);
}
} // namespace halo2::native_device_property_query_detail
#endif
