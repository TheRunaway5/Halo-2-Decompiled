#pragma once
// Boundary adapter for the record sanity check, entered at 14FBC0
// (native_tu15_record_sanity_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the record; the slot the entry PUSH ECX
// makes holds each value a finite test spills, so ECX returns the last one
// (POP ECX); the result in AL (MOV AL,1 or XOR AL,AL) above the rest of EAX
// as the original leaves it (the exponent bits, the kinds, AH from LAHF,
// the mode's address); EDX, XMM0 to XMM3 as the original leaves them; the
// flags of the last instruction that sets them; RET. Each access publishes
// its original instruction.
// HALO2_REFERENCE_RECORD_SANITY=1 restores the translated body.
// HALO2_VERIFY_RECORD_SANITY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_record_sanity_systems.hpp"

namespace halo2::native_record_sanity_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_sanity;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_SANITY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_SANITY");return p && *p;}();
    return value;
}
enum Branch : unsigned {insane, sane, linked, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record sanity verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
// The finite tests: (read, store, reload) and the registers they use.
inline constexpr std::uint32_t finite_pcs[6][3]{
    {0x14fbc9u,0x14fbceu,0x14fbd3u},{0x14fbe6u,0x14fbebu,0x14fbf0u},{0x14fc05u,0x14fc0au,0x14fc0fu},
    {0x14fc22u,0x14fc27u,0x14fc2cu},{0x14fc64u,0x14fc69u,0x14fc6eu},{0x14fc81u,0x14fc86u,0x14fc8bu},
};
template<class State> struct Sanity : Port<State> {
    using Access=logic::record_sanity::Access;
    unsigned step{},bounded{},pair{},zero{};bool marked_read{};
    Sanity(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t slot()const{return this->gp[esp];}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case finite_read:case bounded_read:return finite_pcs[step][0];
        case finite_store:case bounded_store:return finite_pcs[step][1];
        case finite_reload:case bounded_reload:return finite_pcs[step][2];
        case weight:return 0x14fc41u;
        case one:return 0x14fc52u;
        case weight_again:return 0x14fc5au;
        case bounded_value:return bounded++?0x14fcbfu:0x14fca0u;
        case low:return 0x14fca5u;
        case first_kind:return 0x14fcd6u;
        case second_kind:return 0x14fcedu;
        case third_kind:return 0x14fd12u;
        case fourth_kind:return 0x14fd27u;
        case flags:return 0x14fd3cu;
        case pair_range:return pair++?0x14fd51u:0x14fd42u;
        case pair_zero:return zero++?0x14fd74u:0x14fd66u;
        case marked:return 0x14fd82u;
        case link:return 0x14fd8cu;
        case mode:return 0x14fda1u;
        case mode_link:return 0x14fdb4u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        const auto scalar=[](std::uint32_t bits){return logic::sse::scalar(bits);};
        switch(a) {
        case finite_read:case bounded_read:x[step<4u?0:1]=scalar(v);break;                       // MOVSS XMM0 / XMM1
        // MOV reg,[ESP]; AND reg,7F800000; CMP reg,7F800000 (EAX and EDX by turns).
        case finite_reload:case bounded_reload: {
            auto &r=gp[step%2u?edx:eax];
            r=fl.logic(v&logic::exponent_bits,4);fl.minus(r,logic::exponent_bits,4);break;
        }
        case weight:x[0]=scalar(v);x[2]=_mm_setzero_ps();break;                                   // MOVSS XMM0; XORPS XMM2
        case one:x[0]=scalar(v);break;
        case bounded_value:x[3]=scalar(v);break;
        case low:x[1]=scalar(v);break;
        case first_kind:set_low_byte(gp[eax],v);break;
        case second_kind:set_low_byte(gp[edx],v);break;
        case third_kind:case fourth_kind:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);break;         // MOV AX
        case flags:this->test8(v&0xfeu);break;                                                       // TEST byte,FE
        case pair_range:x[1]=scalar(v);break;
        case pair_zero:x[0]=scalar(v);break;
        case marked:this->test8(v&1u);marked_read=true;break;                                       // TEST byte,1
        case link:fl.minus(v,logic::none,4);if(v!=logic::none)this->reached(Branch::linked);break;
        case mode:gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);break;                                 // MOV CX
        case mode_link:fl.minus(v,logic::none,4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void present(std::uint32_t record){this->test32(record);}                                     // TEST ECX,ECX
    void checking(unsigned index){step=index;}
    // COMISS, or UCOMISS; LAHF; TEST AH,44.
    void compared(std::uint32_t a,std::uint32_t b,bool unordered) {
        auto &fl=this->flags;
        fl.scalar_compare(logic::sse::scalar(a),logic::sse::scalar(b),unordered);
        if(unordered) {
            const auto ah=(fl.bits()|2u)&0xffu;
            this->gp[eax]=(this->gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
        }
    }
    // CMP reg,NONE; (TEST reg,reg; CMP reg,limit) as far as the original goes.
    void kind_compared(std::uint32_t value,std::uint32_t none_value,std::int32_t limit,unsigned width) {
        auto &fl=this->flags;
        const auto mask=width==1?0xffu:0xffffu;
        fl.minus(value&mask,none_value&mask,width);
        if((value&mask)==(none_value&mask))return;
        fl.logic(value&mask,width);
        if(logic::signed_of(value,width)<0)return;
        fl.minus(value&mask,std::uint32_t(limit),width);
    }
    // CMP AL,FF; (CMP AL,DL).
    void kinds_compared(std::uint32_t first,std::uint32_t second) {
        this->flags.minus(first&0xffu,0xffu,1);
        if((first&0xffu)!=0xffu)this->flags.minus(first&0xffu,second&0xffu,1);
    }
    void pair_in_range(bool in){set_low_byte(this->gp[edx],in?1u:this->flags.logic(0u,1));}   // MOV DL,1 / XOR DL,DL
    void pair_tested(){this->test8(this->gp[edx]);}                                                // TEST DL,DL
    void mode_address(std::uint32_t address){this->gp[eax]=address;this->test32(address);}       // LEA EAX; TEST EAX,EAX
    // TEST CX,CX; (CMP CX,9; TEST CX,CX).
    void mode_compared(std::uint32_t value) {
        auto &fl=this->flags;
        fl.logic(value&0xffffu,2);
        if(logic::signed_of(value,2)<0)return;
        fl.minus(value&0xffffu,9u,2);
        if(logic::signed_of(value,2)<9)fl.logic(value&0xffffu,2);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0x14fbc0u,gp[ecx]);                                                            // PUSH ECX: the slot
        Memory<Sanity> memory{*this};
        const bool result=logic::record_sanity::sane(memory,*this,gp[ecx]);
        if(result) {
            this->reached(Branch::sane);set_low_byte(gp[eax],1u);gp[ecx]=this->pop(0x14fdbcu);
            this->ret(0x14fdbdu,0);this->publish(0x14fdbdu);
        } else {
            this->reached(Branch::insane);set_low_byte(gp[eax],this->flags.logic(0u,1));gp[ecx]=this->pop(0x14fd94u);
            this->ret(0x14fd95u,0);this->publish(0x14fd95u);
        }
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x14fbc0u,0x14fdbdu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Sanity<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_record_sanity_detail
