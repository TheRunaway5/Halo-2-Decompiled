#pragma once
// Boundary adapter for the object rate, entered at DFE30
// (native_tu15_object_rate_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EDI the handle, the rate's
// address on the stack (RET 4); EBX and ESI saved; DFD20, E65A0 and E3310
// get EAX the handle (DFD20 answers XMM0, the others AL), every register
// each returns coming back; AL the answer (BL, zero, unless rated); ECX and
// EDX as the original leaves them; XMM0 the rate, XMM1 the value, XMM2 the
// ticks; the flags of the last test. Each access publishes its original
// instruction.
// HALO2_REFERENCE_OBJECT_RATE=1 restores the translated body.
// HALO2_VERIFY_OBJECT_RATE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/objects/native_tu15_object_rate_systems.hpp"

namespace halo2::native_object_rate_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_object_rate;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_OBJECT_RATE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_OBJECT_RATE");return p && *p;}();
    return value;
}
enum Branch : unsigned {unrated, refused, still, rated, second_accepted, branch_count};
static_assert(unsigned(logic::object_rate::Outcome::rated)==Branch::rated);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Object rate verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
template<class State,class Value,class First,class Second> struct ObjectRate : Port<State> {
    using Access=logic::object_rate::Access;
    using Kept=logic::object_rate::Kept;
    Value &value_child;
    First &first_child;
    Second &second_child;
    ObjectRate(State &s,std::uint32_t *c,Trace<State> *t,Value &v,First &f,Second &n)
        :Port<State>(s,c,t,{}),value_child(v),first_child(f),second_child(n){}
    std::uint32_t argument()const{return this->gp[esp]+0xcu;}                                   // past the saves
    void refresh(Kept &kept){kept={this->gp[esi],this->gp[edi],lane0(this->s.xmm[1])};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects:return 0xdfe30u;
        case object_data:return 0xdfe36u;
        case object:gp[eax]=3u*(gp[edi]&0xffffu);return 0xdfe43u;                                 // MOV EAX,EDI; AND; LEA
        case tag:return 0xdfe47u;
        case tags:return 0xdfe49u;
        case definition:this->push(0xdfe55u,gp[ebx]);this->push(0xdfe59u,gp[esi]);return 0xdfe5au;   // PUSH EBX; PUSH ESI
        case flags:return 0xdfe65u;
        case zero:return 0xdfe8cu;
        case time:return 0xdfe99u;
        case ticks:return 0xdfe9fu;
        case span_end:return 0xdfea3u;
        case span_start:return 0xdfeabu;
        case rate_slot:return 0xdfeb3u;
        case rate_store:gp[esi]=this->pop(0xdfec3u);return 0xdfec4u;                              // POP ESI
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case objects:gp[ecx]=v;break;
        case object_data:gp[edx]=v;break;
        case object:gp[eax]=v;break;
        case tag:gp[ecx]=v;break;
        case tags:gp[edx]=v;gp[ecx]=this->flags.shift(this->flags.logic(gp[ecx]&0xffffu,4),4u,4);break;   // AND ECX,FFFF; SHL ECX,4
        case definition:gp[esi]=v;break;
        // MOV EAX,[ESI+264]; SHR EAX,1; XOR BL,BL; TEST AL,1.
        case flags:gp[eax]=v>>1;set_low_byte(gp[ebx],0u);this->test8(gp[eax]&1u);break;
        // UCOMISS XMM1,[46A0E8]; LAHF; TEST AH,44.
        case zero:{
            this->flags.scalar_compare(x[1],lane0_only(v),true);
            const auto ah=(this->flags.bits()|2u)&0xffu;
            gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
            break;
        }
        case time:gp[ecx]=v;break;
        case ticks:gp[edx]=logic::widen16(v&0xffffu);break;
        case span_end:x[0]=lane0_only(v);break;
        case rate_slot:gp[eax]=v;break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV EAX,EDI; CALL DFD20; (MOVAPS XMM1,XMM0).
    std::uint32_t value(Kept &kept) {
        this->gp[eax]=kept.handle;
        this->call(0xdfe60u,0xdfe65u,0xdfd20u,value_child);
        this->s.xmm[1]=this->s.xmm[0];
        refresh(kept);
        return kept.value;
    }
    // MOV EAX,EDI; CALL E65A0 (or E3310); TEST AL,AL.
    std::uint32_t accepted(Kept &kept,std::uint32_t which) {
        this->gp[eax]=kept.handle;
        if(!which)this->call(0xdfe78u,0xdfe7du,0xe65a0u,first_child);
        else this->call(0xdfe83u,0xdfe88u,0xe3310u,second_child);
        this->test8(this->gp[eax]);
        if(which && (this->gp[eax]&0xffu))this->reached(Branch::second_accepted);
        refresh(kept);
        return this->gp[eax];
    }
    // SUBSS; CVTSI2SS XMM2,EDX; MULSS; MULSS.
    void rated(std::uint32_t rate) {
        auto &x=this->s.xmm;
        x[2]=_mm_move_ss(x[2],lane0_only(logic::converted(std::int32_t(this->gp[edx]))));
        x[0]=lane0_only(rate);
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::object_rate::Outcome;
        Memory<ObjectRate> memory{*this};
        const auto outcome=logic::object_rate::of(memory,*this,gp[edi]);
        this->reached(unsigned(outcome));
        if(outcome==Outcome::rated) {
            set_low_byte(gp[eax],1u);gp[ebx]=this->pop(0xdfecau);
            this->ret(0xdfecbu,4u);this->publish(0xdfecbu);
        } else {
            gp[esi]=this->pop(0xdfeceu);set_low_byte(gp[eax],gp[ebx]);gp[ebx]=this->pop(0xdfed1u);
            this->ret(0xdfed2u,4u);this->publish(0xdfed2u);
        }
    }
};
template<class State,class Reference,class Value,class First,class Second>
bool bridge(State &s,Reference reference,Value value,First first,Second second) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xdfe30u,0xdfed2u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return ObjectRate<State,Value,First,Second>(v,c,t,value,first,second);});
    return true;
}
} // namespace halo2::native_object_rate_detail
