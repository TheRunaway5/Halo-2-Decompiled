#pragma once
// Boundary adapter for the director value, entered at 189F20
// (native_tu15_director_value_systems.hpp). Its callers and the child keep
// the original ABI, which the adapter owns: EAX the record index (RET); EBX
// saved around CAD60, which gets EAX the vehicle and BX the word and
// answers XMM0, every register it returns coming back; EAX, ECX and EDX as
// the original leaves them (LAHF in AH); XMM0 the answer, XMM1 as the
// quotient's tests leave it; the flags of the last comparison. Each access
// publishes its original instruction.
// HALO2_REFERENCE_DIRECTOR_VALUE=1 restores the translated body.
// HALO2_VERIFY_DIRECTOR_VALUE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/vehicles/native_tu15_director_value_systems.hpp"

namespace halo2::native_director_value_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_director_value;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_DIRECTOR_VALUE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_DIRECTOR_VALUE");return p && *p;}();
    return value;
}
enum Branch : unsigned {unbound, unscaled, out_of_bounds, scaled, branch_count};
static_assert(unsigned(logic::director_value::Outcome::scaled)==Branch::scaled);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Director value verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
template<class State,class Factor> struct DirectorValue : Port<State> {
    using Access=logic::director_value::Access;
    Factor &factor_child;
    bool saved{};
    DirectorValue(State &s,std::uint32_t *c,Trace<State> *t,Factor &f):Port<State>(s,c,t,{}),factor_child(f){}
    std::uint32_t local(std::uint32_t k)const{return this->gp[esp]+(saved?4u:0u)+4u*k;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case records:return 0x189f20u;
        case fallback:gp[eax]=fl.product(gp[eax],logic::record_size);return 0x189f2cu;              // IMUL EAX,EAX,94
        case vehicle:gp[ecx]=gp[eax]+gp[ecx]+logic::record::vehicle;return 0x189f38u;               // LEA ECX,[EAX+ECX+14]
        case word:this->push(0x189f46u,gp[ebx]);saved=true;return 0x189f47u;                         // PUSH EBX
        case objects:gp[edx]=3u*(gp[eax]&0xffffu);return 0x189f56u;
        case object_data:return 0x189f5cu;
        case object:return 0x189f5fu;
        case tag:return 0x189f63u;
        case tags:return 0x189f65u;
        case definition:gp[ecx]=fl.shift(fl.logic(gp[ecx]&0xffffu,4),4u,4);return 0x189f74u;        // AND ECX,FFFF; SHL ECX,4
        case value:return 0x189f78u;
        case kept_store:return 0x189f80u;
        case kept_again_store:return 0x189f86u;
        case one:return 0x189f91u;
        case kept:return 0x189f9fu;
        case lower:return 0x189fabu;
        case upper:return 0x189fb4u;
        case kept_again:return 0x189fc1u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case records:gp[ecx]=v;break;
        case fallback:x[0]=lane0_only(v);break;
        case vehicle:gp[eax]=v;gp[esp]-=8u;this->compare32(v,logic::none);break;                     // SUB ESP,8; CMP EAX,-1
        case word:gp[ebx]=(gp[ebx]&0xffff0000u)|(v&0xffffu);break;
        case objects:case object_data:gp[ecx]=v;break;
        case object:gp[edx]=v;break;
        case tag:gp[ecx]=v;break;
        case tags:gp[edx]=v;break;
        case definition:gp[ecx]=v;break;
        case value:x[0]=lane0_only(v);break;
        // UCOMISS XMM0,[46A0D0]; LAHF; TEST AH,44; POP EBX.
        case one:{
            this->flags.scalar_compare(x[0],lane0_only(v),true);
            const auto ah=(this->flags.bits()|2u)&0xffu;
            gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
            gp[ebx]=this->pop(0x189f9cu);saved=false;
            break;
        }
        case kept:x[1]=lane0_only(v);break;
        case lower:this->flags.scalar_compare(x[0],lane0_only(v),false);break;                    // COMISS XMM0,[46A7C4]
        case upper:x[1]=lane0_only(v);this->flags.scalar_compare(x[1],x[0],false);break;           // COMISS XMM1,XMM0
        case kept_again:x[0]=lane0_only(v);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    std::uint32_t factor(std::uint32_t,std::uint32_t) {
        this->call(0x189f8cu,0x189f91u,0xcad60u,factor_child);
        return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(this->s.xmm[0])));
    }
    // DIVSS XMM1,XMM0; MOVAPS XMM0,XMM1.
    void divided(std::uint32_t quotient){this->s.xmm[1]=lane0_only(quotient);this->s.xmm[0]=this->s.xmm[1];}
    void answered(std::uint32_t value_bits){this->s.xmm[0]=lane0_only(value_bits);}
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        Memory<DirectorValue> memory{*this};
        const auto outcome=logic::director_value::of(memory,*this,gp[eax]);
        this->reached(unsigned(outcome));
        gp[esp]=fl.plus(gp[esp],8u,4);                                                             // ADD ESP,8
        this->ret(0x189fcau,0u);this->publish(0x189fcau);
    }
};
template<class State,class Reference,class Factor> bool bridge(State &s,Reference reference,Factor factor) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x189f20u,0x189fcau,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return DirectorValue<State,Factor>(v,c,t,factor);});
    return true;
}
} // namespace halo2::native_director_value_detail
