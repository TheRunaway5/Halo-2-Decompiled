#pragma once
// TU1.5 cluster-portal DFS. The native driver retains the published bank ABI
// until its callers and the sphere/portal predicate have native owners too.
// PortalTest is the explicit14CEA0 dependency; Recurse enters the same driver.
#include "physics/runtime/native_tu15_matrix_leaves.hpp"

namespace halo2::native_bsp_cluster_detail {
inline bool reference() {
    static const bool enabled=[] {
        const auto *value=std::getenv("HALO2_REFERENCE_BSP_CLUSTER_FLOOD");
        return value && *value;
    }();
    return enabled;
}
inline std::uint32_t signed_word(std::uint32_t value) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));
}
// Only exception ABI publication needs flags from integer work between memory
// accesses. Keep that state lazy; successful traversal uses native arithmetic
// and materializes flags only at real predicate/return boundaries.
template<class Flags> struct FaultFlags {
    Flags &flags;
    enum Kind { none, logic, plus, minus, increment } kind=none;
    std::uint32_t a{},b{};unsigned width=4;bool saved_carry{};
    void record(Kind k,std::uint32_t x,std::uint32_t y=0,unsigned n=4) {kind=k;a=x;b=y;width=n;}
    void clear(){kind=none;}
    bool carry()const {
        const auto mask=Flags::mask(width);
        switch(kind) {
        case logic:return false;
        case plus:return std::uint64_t(a&mask)+(b&mask)>mask;
        case minus:return (a&mask)<(b&mask);
        case increment:return saved_carry;
        default:return flags.cf;
        }
    }
    void record_increment(std::uint32_t value) {saved_carry=carry();record(increment,value,1);}
    void materialize() {
        switch(kind) {
        case logic:flags.logic(a,width);break;
        case plus:flags.plus(a,b,width);break;
        case minus:flags.minus(a,b,width);break;
        case increment:flags.plus(a,1,width);flags.cf=saved_carry;break;
        default:break;
        }
        clear();
    }
};
template<class State, class PortalTest, class Recurse>
bool run(State &s, PortalTest portal_test, Recurse recurse) {
    if(reference() || s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    std::uint32_t *cursor=nullptr;
    if(s.services.instruction) {
        if(!s.literal_service_sites || !s.instruction_cursor || !s.instruction_cursor->position)return false;
        const auto *policy=s.instruction_cursor;
        if((policy->first_trace && *policy->first_trace) || (policy->second_trace && *policy->second_trace) ||
            (s.observe && !policy->observation_only_stores))return false;
        cursor=policy->position;
    }
    const auto at=[&](std::uint32_t pc){if(cursor)*cursor=pc;};
    const auto read=[&](std::uint32_t pc,std::uint32_t address,unsigned size=4) {
        at(pc);return physics_step_detail::r(s.memory,address,size);
    };
    const auto store=[&](std::uint32_t pc,std::uint32_t address,unsigned size,std::uint32_t value) {
        at(pc);physics_step_detail::w(s.memory,s.observe,pc,address,size,value);
    };
    const auto push=[&](std::uint32_t pc,std::uint32_t value) {s.gp[4]-=4;store(pc,s.gp[4],4,value);};
    const auto pop=[&](std::uint32_t pc) {const auto value=read(pc,s.gp[4]);s.gp[4]+=4;return value;};
    const auto low_word=[&](unsigned reg,std::uint32_t value) {s.gp[reg]=(s.gp[reg]&0xffff0000u)|(value&0xffffu);};
    auto &gp=s.gp;
    using Flags=std::remove_cvref_t<decltype(s.flags)>;
    FaultFlags<Flags> fault_flags{s.flags};
    using Pending=FaultFlags<Flags>;
    try {

    // Establish the original caller-visible frame and saved nonvolatile words.
    push(0x14d0e0u,gp[1]);
    gp[2]=read(0x14d0e1u,0x4ecc18u);
    push(0x14d0e7u,gp[3]);low_word(3,read(0x14d0e8u,gp[4]+12u,2));
    push(0x14d0edu,gp[5]);gp[5]=read(0x14d0eeu,gp[2]+0xa0u);
    push(0x14d0f4u,gp[6]);gp[0]=signed_word(gp[3]);
    fault_flags.record(Pending::plus,gp[0]*0xb0u,gp[5]);gp[6]=gp[5]+gp[0]*0xb0u;
    gp[5]=read(0x14d102u,gp[4]+0x20u);
    low_word(1,gp[5]);--gp[5];
    fault_flags.record(Pending::logic,gp[1]&0xffffu,0,2);
    push(0x14d10du,gp[7]);gp[7]=read(0x14d10eu,gp[4]+0x28u);
    store(0x14d112u,gp[4]+0x10u,4,gp[2]);
    if(std::bit_cast<std::int16_t>(std::uint16_t(gp[1]))>0) {
        store(0x14d118u,gp[7],2,gp[3]);fault_flags.record(Pending::plus,gp[7],2);gp[7]+=2u;
    }
    gp[1]=read(0x14d11eu,0x4f3ce4u);
    const auto visited=read(0x14d124u,0x4f3ce8u+gp[0]*4u);
    fault_flags.record(Pending::minus,visited,gp[1]);
    if(visited!=gp[1])
        store(0x14d12du,0x4f3ce8u+gp[0]*4u,4,gp[1]);
    gp[1]=read(0x14d134u,gp[6]+0x8cu);
    s.flags.logic(gp[1],4);fault_flags.clear();
    gp[0]=1;
    store(0x14d141u,gp[4]+0x28u,4,1);
    store(0x14d145u,gp[4]+0x24u,4,0);
    if(std::bit_cast<std::int32_t>(gp[1])>0) {
        gp[0]=0;
        fault_flags.record(Pending::logic,0);
        for(bool first=true;;first=false) {
            if(!first) {
                gp[2]=read(0x14d157u,gp[4]+0x10u);
                low_word(3,read(0x14d15bu,gp[4]+0x18u,2));
            }
            gp[1]=read(0x14d160u,gp[6]+0x90u);
            low_word(0,read(0x14d166u,gp[1]+gp[0]*2u,2));
            gp[2]=read(0x14d16au,gp[2]+0x60u);
            gp[1]=gp[2]+signed_word(gp[0])*36u;
            low_word(2,read(0x14d176u,gp[1],2));
            fault_flags.record(Pending::minus,gp[2]&0xffffu,gp[3]&0xffffu,2);
            if(std::uint16_t(gp[2])==std::uint16_t(gp[3]))
                low_word(3,read(0x14d17eu,gp[1]+2u,2));
            else gp[3]=gp[2];
            gp[2]=read(0x14d186u,0x4f3ce4u);gp[1]=signed_word(gp[3]);
            s.flags.minus(read(0x14d18fu,0x4f3ce8u+gp[1]*4u),gp[2],4);
            fault_flags.clear();
            if(!s.flags.zf) {
                // The unconverted predicate gets its exact register/stack ABI.
                // Its real result, FP state and source side effects are retained.
                gp[1]=read(0x14d198u,gp[4]+0x20u);
                gp[2]=read(0x14d19cu,gp[4]+0x10u);
                push(0x14d1a0u,gp[1]);gp[1]=read(0x14d1a1u,gp[4]+0x20u);
                push(0x14d1a5u,0x14d1aau);portal_test(s);
                s.flags.logic(gp[0]&255u,1);
                if(!s.flags.zf) {
                    gp[2]=read(0x14d1aeu,gp[4]+0x20u);
                    gp[0]=read(0x14d1b2u,gp[4]+0x1cu);
                    push(0x14d1b6u,gp[7]);push(0x14d1b7u,gp[5]);push(0x14d1b8u,gp[2]);
                    push(0x14d1b9u,gp[0]);push(0x14d1bau,gp[3]);push(0x14d1bbu,0x14d1c0u);
                    recurse(s);
                    gp[2]=read(0x14d1c0u,gp[4]+0x28u);
                    gp[1]=signed_word(gp[0]);gp[7]+=gp[1]*2u;
                    fault_flags.record(Pending::plus,gp[2],gp[0]);gp[2]+=gp[0];
                    store(0x14d1ccu,gp[4]+0x28u,4,gp[2]);
                    fault_flags.record(Pending::minus,gp[5],gp[0]);gp[5]-=gp[0];
                }
            }
            // The portal count and index are deliberately re-read: the
            // predicate/recursive owner can update the retained source bank.
            gp[0]=read(0x14d1d2u,gp[4]+0x24u);
            gp[1]=read(0x14d1d6u,gp[6]+0x8cu);fault_flags.record_increment(gp[0]);++gp[0];
            store(0x14d1ddu,gp[4]+0x24u,4,gp[0]);gp[0]=signed_word(gp[0]);
            s.flags.minus(gp[0],gp[1],4);
            fault_flags.clear();
            if(s.flags.sf==s.flags.of)break;
        }
        low_word(0,read(0x14d1ecu,gp[4]+0x28u,2));
    }
    gp[7]=pop(0x14d1f1u);gp[6]=pop(0x14d1f2u);gp[5]=pop(0x14d1f3u);
    gp[3]=pop(0x14d1f4u);gp[1]=pop(0x14d1f5u);
    (void)pop(0x14d1f6u);gp[4]+=20u;
    return true;
    } catch(...) {fault_flags.materialize();throw;}
}
} // namespace halo2::native_bsp_cluster_detail
