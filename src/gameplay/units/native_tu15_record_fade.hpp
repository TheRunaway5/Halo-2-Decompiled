#pragma once
// Boundary adapter for the record fade, entered at 17FB60
// (native_tu15_record_fade_systems.hpp). The callers and the children keep
// the original ABI, which the adapter owns: EAX the handle; AL the result;
// ECX/EBX/ESI/EDI saved (the ECX slot holds the fade value); ESI the record,
// EDI the array, CL the flags, BL the result; 140310 gets EBX the owner and
// EDI the expiry owner; 32CC0C gets the fade value on the logical x87 stack
// (FLD m32, which a full stack refuses at admission) and returns EAX; every
// register a child returns comes back; XMM0-XMM2 and the flags as the last
// comparison leaves them (UCOMISS's LAHF in AH); each access publishes its
// original instruction.
// HALO2_REFERENCE_RECORD_FADE=1 restores the translated body.
// HALO2_VERIFY_RECORD_FADE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_record_fade_systems.hpp"
#include <bit>
#include <emmintrin.h>

namespace halo2::native_record_fade_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_record_fade;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_RECORD_FADE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_RECORD_FADE");return p && *p;}();
    return value;
}
enum Branch : unsigned {held, active, zero_lifetime, expired, counted, uncounted, chain_counted, chain_uncounted,
    no_lifetime, no_fade, not_fading, below_floor, above_limit, within_limit, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Record fade verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline __m128 with_lane0(__m128 x,std::uint32_t bits){return _mm_move_ss(x,lane0_only(bits));}
inline constexpr bool backend=logic::sse::available && logic::scalar::available;
inline constexpr Edge fade_edges[]{
    {0x17fb95u,0x17fc85u,held},{0x17fb95u,0x17fb9fu,active},
    {0x17fb9fu,0x17fc12u,zero_lifetime},{0x17fb9fu,0x17fbbau,expired},{0x17fb9fu,0x17fbfdu,expired},
    {0x17fb9fu,0x17fbbau,counted},{0x17fb9fu,0x17fbfdu,uncounted},
    {0x17fbe1u,0x17fbecu,chain_counted},{0x17fbe1u,0x17fbf5u,chain_uncounted},
    {0x17fc12u,0x17fc85u,no_lifetime},{0x17fc1cu,0x17fc85u,no_fade},{0x17fc2fu,0x17fc85u,not_fading},
    {0x17fc3du,0x17fc4au,below_floor},{0x17fc5du,0x17fc6bu,above_limit},{0x17fc5du,0x17fc65u,within_limit},
};
template<class State,class Expire,class Convert> struct Fade : Port<State> {
    using Access=logic::record_fade::Access;
    using Local=logic::record_fade::Local;
    Expire &expire_child;Convert &convert_child;
    std::uint32_t frame{},x87_value{};
    Fade(State &s,std::uint32_t *c,Trace<State> *t,Expire &e,Convert &v):Port<State>(s,c,t,fade_edges),expire_child(e),convert_child(v){}
    std::uint32_t local(Local)const{return frame-4u;}
    void set_cl(std::uint32_t v){this->gp[ecx]=(this->gp[ecx]&0xffffff00u)|(v&0xffu);}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // PUSH ECX, EBX, ESI; AND EAX,FFFF; PUSH EDI.
        case records_table:
            this->push(0x17fb60u,gp[ecx]);this->push(0x17fb61u,gp[ebx]);this->push(0x17fb62u,gp[esi]);
            gp[eax]=this->flags.logic(gp[eax]&0xffffu,4);this->push(0x17fb68u,gp[edi]);return 0x17fb69u;
        case data:return 0x17fb6fu;
        case game_time:return 0x17fb79u;
        case now:return 0x17fb7eu;
        case start:return 0x17fb81u;
        case flags:return 0x17fb88u;
        case seconds:return 0x17fb8bu;
        // XOR BL,BL; TEST CL,2.
        case fade_reset:
            gp[ebx]=(gp[ebx]&0xffffff00u)|this->flags.logic(0,1);this->test8(gp[ecx]&logic::flag::held);return 0x17fb95u;
        case lifetime:return 0x17fb9fu;
        case counters:return 0x17fbbau;
        case flags_clear:return 0x17fbc3u;
        case count_read:case count_store:return 0x17fbc6u;
        case chain_next:return 0x17fbccu;
        case chain_data:return 0x17fbd4u;
        case chain_flags:return 0x17fbe1u;
        case chain_flags_clear:set_cl(this->flags.logic(gp[ecx]&0xfeu,1));return 0x17fbecu;          // AND CL,FE
        case chain_count_read:case chain_count_store:return 0x17fbefu;
        case chain_after:return 0x17fbf5u;
        case owner:return 0x17fbfdu;
        case expire_owner:return 0x17fc00u;
        case lifetime_again:return 0x17fc12u;
        case fade_time:return 0x17fc1cu;
        case lifetime_remaining:return 0x17fc26u;
        case fade_time_again:return 0x17fc2fu;
        case fade_scale:return 0x17fc3du;
        case fade_limit:return 0x17fc52u;
        case fade_store_zero:return 0x17fc4au;
        case fade_store_limit:return 0x17fc5du;
        case fade_store_value:return 0x17fc65u;
        case fade_for_x87:return 0x17fc6bu;
        case fade_word:return 0x17fc74u;
        case fade_word_store:return 0x17fc82u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case records_table:gp[edi]=v;break;
        // MOV ESI,[EDI+44]; SHL EAX,6; ADD EAX,ESI; MOV ESI,EAX.
        case data:gp[esi]=v;gp[eax]=this->flags.plus(this->flags.shift(gp[eax],logic::record_shift,4),gp[esi],4);gp[esi]=gp[eax];break;
        case game_time:case chain_next:case chain_after:
            gp[eax]=v;if(a!=game_time)this->compare32(v,logic::none);break;
        case now:gp[ecx]=v;break;
        case start:gp[ecx]=this->flags.minus(gp[ecx],v,4);break;
        case flags:case chain_flags:set_cl(v);if(a==chain_flags)this->test8(v&logic::flag::counted);break;
        case lifetime:x[0]=lane0_only(v);break;
        case counters:gp[edx]=v;set_cl(this->flags.logic(gp[ecx]&0xfeu,1));break;                    // AND CL,FE
        case count_read:case chain_count_read:this->flags.increment(v,4,true);break;                   // DEC
        // MOV ECX,[EDI+44]; AND EAX,FFFF; SHL EAX,6; ADD EAX,ECX.
        case chain_data:gp[ecx]=v;gp[eax]=this->flags.plus(this->flags.shift(this->flags.logic(gp[eax]&0xffffu,4),logic::record_shift,4),gp[ecx],4);break;
        case owner:gp[ebx]=v;break;
        case expire_owner:gp[edi]=v;break;
        case lifetime_again:case fade_time:x[0]=lane0_only(v);this->flags.scalar_compare(x[0],x[1],false);break;
        case fade_time_again:x[2]=lane0_only(v);this->flags.scalar_compare(x[2],x[0],false);break;
        case fade_limit:x[1]=lane0_only(v);this->flags.scalar_compare(x[0],x[1],false);break;
        case fade_for_x87:x87_value=v;break;
        // MOV EDX,[ESI+24]; SHL EAX,18; AND EDX,FFFFFF; OR EAX,EDX.
        case fade_word:
            gp[edx]=v;gp[eax]=this->flags.shift(gp[eax],24u,4);gp[edx]=this->flags.logic(gp[edx]&0xffffffu,4);
            gp[eax]=this->flags.logic(gp[eax]|gp[edx],4);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void age_computed(std::uint32_t age){this->s.xmm[2]=with_lane0(this->s.xmm[2],age);}
    // XORPS XMM1; UCOMISS XMM0,XMM1; LAHF; TEST AH,44; (COMISS XMM0,XMM2).
    void aged(const logic::record_fade::Age &a) {
        auto &gp=this->gp;
        auto &x=this->s.xmm;
        x[1]=_mm_setzero_ps();
        this->flags.scalar_compare(x[0],x[1],true);
        const auto ah=(this->flags.bits()|2u)&0xffu;
        gp[eax]=(gp[eax]&0xffff00ffu)|(ah<<8);this->test8(ah&0x44u);
        if(!a.equal_zero)this->flags.scalar_compare(x[0],x[2],false);
        if(a.reached)this->test8(gp[ecx]&logic::flag::counted);                                    // TEST CL,1
    }
    void expire(std::uint32_t,std::uint32_t) {
        this->call(0x17fc06u,0x17fc0bu,0x140310u,expire_child);
    }
    void remaining(std::uint32_t value){this->s.xmm[0]=lane0_only(value);}
    void valued(std::uint32_t value) {
        auto &x=this->s.xmm;
        x[0]=lane0_only(value);this->flags.scalar_compare(x[1],x[0],false);
    }
    // FLD [ESP+C]; CALL 32CC0C.
    std::uint32_t converted(std::uint32_t &record) {
        this->s.fp.push(static_cast<long double>(std::bit_cast<float>(x87_value)));
        this->call(0x17fc6fu,0x17fc74u,0x32cc0cu,convert_child);
        record=this->gp[esi];
        return this->gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        frame=gp[esp];
        Memory<Fade> memory{*this};
        if(logic::record_fade::update(memory,*this,gp[eax])==logic::record_fade::Exit::expired) {
            gp[edi]=this->pop(0x17fc0bu);gp[esi]=this->pop(0x17fc0cu);gp[eax]=(gp[eax]&0xffffff00u)|1u;
            gp[ebx]=this->pop(0x17fc0fu);gp[ecx]=this->pop(0x17fc10u);
            this->ret(0x17fc11u,0);this->publish(0x17fc11u);
        } else {
            gp[edi]=this->pop(0x17fc85u);gp[esi]=this->pop(0x17fc86u);gp[eax]=(gp[eax]&0xffffff00u)|(gp[ebx]&0xffu);
            gp[ebx]=this->pop(0x17fc89u);gp[ecx]=this->pop(0x17fc8au);
            this->ret(0x17fc8bu,0);this->publish(0x17fc8bu);
        }
    }
};
template<class State,class Reference,class Expire,class Convert>
bool bridge(State &s,Reference reference,Expire expire,Convert convert) {
    if(!backend)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>=8u)return false;
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x17fb60u,0x17fc8bu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Fade<State,Expire,Convert>(v,c,t,expire,convert);});
    return true;
}
} // namespace halo2::native_record_fade_detail
