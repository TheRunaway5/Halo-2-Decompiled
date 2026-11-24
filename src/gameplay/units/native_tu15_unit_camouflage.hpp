#pragma once
// Boundary adapter for the unit camouflage, entered at C8CF0
// (native_tu15_unit_camouflage_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: EAX the unit handle (RET);
// EBX, EBP, ESI and EDI saved; C1E90 gets EDX the bit and the bits pushed
// (RET 4) and answers AL, B7E50 the handle and the notice pushed (RET 8),
// every register each returns coming back; EAX the flags past bit 4, ECX
// and EDX as the original leaves them; XMM0 the second fade, XMM1 zero,
// XMM2 one (or the falling second step), XMM3 the falling camouflage step;
// the flags of the second fade's COMISS. Each access publishes its original
// instruction.
// HALO2_REFERENCE_UNIT_CAMOUFLAGE=1 restores the translated body.
// HALO2_VERIFY_UNIT_CAMOUFLAGE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/units/native_tu15_unit_camouflage_systems.hpp"

namespace halo2::native_unit_camouflage_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_camouflage;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_CAMOUFLAGE");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_CAMOUFLAGE");return p && *p;}();
    return value;
}
enum Branch : unsigned {forced, allied, full, gone, notified, second_full, second_gone, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit camouflage verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline __m128 lane0_only(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline std::uint32_t lane0(__m128 x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(x)));}
// A fade's instructions while rising ([1]) and falling ([0]).
struct FadeSites {std::uint32_t rate,tick,value,store,clamp;};
inline constexpr FadeSites camouflage_sites[2]{{0xc8db5u,0xc8dbdu,0xc8dc2u,0xc8dd4u,0xc8ddeu},{0xc8d86u,0xc8d8eu,0xc8d93u,0xc8da1u,0xc8dabu}};
inline constexpr FadeSites second_sites[2]{{0xc8e4au,0xc8e3eu,0xc8e42u,0xc8e59u,0xc8e63u},{0xc8e14u,0xc8e10u,0xc8e1cu,0xc8e27u,0xc8e32u}};
template<class State,class Allied,class Notify> struct UnitCamouflage : Port<State> {
    using Access=logic::unit_camouflage::Access;
    using Kept=logic::unit_camouflage::Kept;
    Allied &allied_child;
    Notify &notify_child;
    bool rising{},second_fade{},second_rising{},early_edi{};
    UnitCamouflage(State &s,std::uint32_t *c,Trace<State> *t,Allied &a,Notify &n)
        :Port<State>(s,c,t,{}),allied_child(a),notify_child(n){}
    const FadeSites &fade()const{return second_fade?second_sites[second_rising?1:0]:camouflage_sites[rising?1:0];}
    void refresh(Kept &kept) {
        auto &gp=this->gp;
        kept={gp[ebp],gp[edi],gp[esi],gp[ebx],lane0(this->s.xmm[1]),lane0(this->s.xmm[2])};
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case objects:return 0xc8cf2u;
        // AND EAX,FFFF; PUSH ESI; PUSH EDI; LEA EDI,[EAX+EAX*2]; (SHL EDI,2).
        case object_data:
            gp[eax]&=0xffffu;this->push(0xc8cfdu,gp[esi]);this->push(0xc8cfeu,gp[edi]);gp[edi]=logic::row_size*gp[eax];
            return 0xc8d02u;
        case object:return 0xc8d08u;
        case player:return 0xc8d0cu;
        case game_state:return 0xc8d15u;
        case game_mode:return 0xc8d1bu;
        case session_flag:return 0xc8d21u;
        case team:return 0xc8d2au;
        case alliances:gp[edx]=logic::widen16(gp[eax]&0xffffu);return 0xc8d45u;                 // MOVSX EDX,AX
        case forced_flags:case forced_store:return 0xc8d60u;
        case flags:return 0xc8d67u;
        case one:this->s.xmm[1]=_mm_setzero_ps();return 0xc8d70u;                                 // XORPS XMM1,XMM1
        case time:return 0xc8d78u;
        case rate:case second_rate:return fade().rate;
        case tick:case second_tick:return fade().tick;
        case camouflage:case second:return fade().value;
        case camouflage_store:case second_store:return fade().store;
        case camouflage_clamp:return fade().clamp;
        case second_clamp:
            if(second_rising){gp[edi]=this->pop(0xc8e31u);early_edi=true;}                         // POP EDI
            return fade().clamp;
        case object_data_again:return 0xc8de6u;
        case object_again:return 0xc8de9u;
        case attachment:return 0xc8dedu;
        case second_flags:second_fade=true;return 0xc8e03u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case objects:gp[ebp]=v;break;
        case object_data:gp[eax]=v;break;
        case object:gp[esi]=v;break;
        case game_state:gp[ecx]=v;break;
        case session_flag:set_low_byte(gp[eax],v);break;
        case team:gp[eax]=(gp[eax]&0xffff0000u)|(v&0xffffu);break;
        case alliances:gp[eax]=v;break;
        case flags:gp[ecx]=v>>3;rising=v&logic::flag::camouflaged;break;                        // MOV ECX; SHR ECX,3
        case one:x[2]=lane0_only(v);break;
        case time:gp[ebx]=v;break;
        case rate:if(rising)x[0]=lane0_only(v);else x[3]=lane0_only(v);break;
        case camouflage:if(!rising)x[0]=lane0_only(v);break;
        case object_data_again:gp[edx]=v;break;
        case object_again:gp[edi]=v;break;
        case attachment:gp[eax]=v;break;
        case second_flags:gp[eax]=v>>4;second_rising=v&logic::flag::second_rising;break;       // MOV EAX; SHR EAX,4
        case second_tick:if(second_rising)x[0]=lane0_only(v);else x[2]=lane0_only(v);break;
        case second:if(!second_rising)x[0]=lane0_only(v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case forced_store:this->reached(Branch::forced);break;
        // The value's COMISS against one (rising) or of zero against it (falling).
        case camouflage_store:case second_store:
            x[0]=lane0_only(v);
            if(a==second_store) {
                if(second_rising)this->flags.scalar_compare(x[0],x[2],false);
                else this->flags.scalar_compare(x[1],x[0],false);
            }
            break;
        case camouflage_clamp:this->reached(rising?Branch::full:Branch::gone);break;
        case second_clamp:this->reached(second_rising?Branch::second_full:Branch::second_gone);break;
        default:break;
        }
    }
    // A step: XMM3 (the falling camouflage) or XMM2 (the falling second fade); rising, XMM0.
    void stepped(std::uint32_t step) {
        auto &x=this->s.xmm;
        const bool up=second_fade?second_rising:rising;
        if(up)x[0]=lane0_only(step);else if(second_fade)x[2]=lane0_only(step);else x[3]=lane0_only(step);
    }
    void ticked(const Kept &kept){this->gp[ebx]=kept.tick;}                                       // ADD EBX,4
    // ADD EAX,C4; ADD EDX,10; PUSH EAX; CALL C1E90; NEG AL; SBB AL,AL; INC AL.
    bool allied(Kept &kept,std::uint32_t bit,std::uint32_t bits) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.plus(bits-logic::alliance_bits,logic::alliance_bits,4);
        gp[edx]=fl.plus(bit-logic::team_one_row,logic::team_one_row,4);
        this->push(0xc8d52u,gp[eax]);
        this->call(0xc8d53u,0xc8d58u,0xc1e90u,allied_child);
        const bool answer=gp[eax]&0xffu;
        set_low_byte(gp[eax],answer?0u:1u);
        if(answer)this->reached(Branch::allied);
        refresh(kept);
        return answer;
    }
    // CMP EAX,-1; PUSH 800000; PUSH EAX; CALL B7E50.
    void notify(Kept &kept,std::uint32_t handle,std::uint32_t notice) {
        this->compare32(handle,logic::none);
        this->push(0xc8df8u,notice);this->push(0xc8dfdu,handle);
        this->call(0xc8dfeu,0xc8e03u,0xb7e50u,notify_child);
        this->reached(Branch::notified);
        refresh(kept);
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xc8cf0u,gp[ebx]);this->push(0xc8cf1u,gp[ebp]);
        Memory<UnitCamouflage> memory{*this};
        logic::unit_camouflage::update(memory,*this,gp[eax]);
        const auto pop_pc=early_edi?0xc8e39u:0xc8e6bu;
        if(!early_edi)gp[edi]=this->pop(pop_pc);
        const auto rest=early_edi?0xc8e3au:0xc8e6cu;
        gp[esi]=this->pop(rest);gp[ebp]=this->pop(rest+1u);gp[ebx]=this->pop(rest+2u);
        this->ret(rest+3u,0u);this->publish(rest+3u);
    }
};
template<class State,class Reference,class Allied,class Notify> bool bridge(State &s,Reference reference,Allied allied,Notify notify) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc8cf0u,0xc8e6fu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return UnitCamouflage<State,Allied,Notify>(v,c,t,allied,notify);});
    return true;
}
} // namespace halo2::native_unit_camouflage_detail
