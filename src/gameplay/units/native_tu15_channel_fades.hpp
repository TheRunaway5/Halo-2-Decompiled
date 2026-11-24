#pragma once
// Boundary adapter for the channel fades, C8390
// (native_tu15_channel_fades_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: EAX the object (RET), the answer
// in AL; ECX pushed (its top byte the moved flag), EBP, ESI and EDI saved (EBX
// past the fixed test); D3150 (EBX the object), D05A0 and E8E10 (ECX 1E, the
// object pushed, RET 4) direct, every register each returns coming back;
// XMM0 1, XMM1 0 and the fades' registers as the original leaves them. Each
// access publishes its original instruction.
// HALO2_REFERENCE_CHANNEL_FADES=1 restores the translated body.
// HALO2_VERIFY_CHANNEL_FADES=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_channel_fades_systems.hpp"

namespace halo2::native_channel_fades_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_channel_fades;
using Access=logic::channel_fades::Access;
using Kept=logic::channel_fades::Kept;
using Outcome=logic::channel_fades::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_CHANNEL_FADES");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_CHANNEL_FADES");return v;}
enum Branch : unsigned {fixed, updated, noticed, faded_in, faded_out, stopped, branch_count};
static_assert(unsigned(Outcome::updated)==Branch::updated);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Channel fades"};return v;}
inline __m128 lane0(std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));}
inline void set_high_byte(std::uint32_t &reg,std::uint32_t value){reg=(reg&0xffff00ffu)|((value&0xffu)<<8);}
inline constexpr std::uint32_t sites[]{0xc8391u,0xc8397u,0xc839au,0xc83adu,0xc83b1u,0xc83bbu,0xc83bfu,0xc83cdu,0xc83e4u,0xc83f1u,
    0xc83f7u,0xc83fdu,0xc8409u,0xc8412u,0xc8412u,0xc8427u,0xc8437u,0xc843du,0xc8450u,0xc8465u,0xc8470u,0xc847cu,0xc8485u,0xc8497u,
    0u,0u,0u,0u,0u,0u,0u,0u,0xc8573u,0xc8578u,0xc8587u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::moved_load)+1u);
// The fades' instructions, fading in and out.
inline constexpr std::uint32_t weight_sites[2]{0xc8511u,0xc84a9u},duration_sites[2]{0xc8523u,0xc84bfu},ticks_sites[2]{0xc852cu,0xc84c8u},
    duration_again_sites[2]{0xc8534u,0xc84d0u},weight_again_sites[2]{0xc853fu,0xc84dbu},final_sites[2]{0xc8560u,0xc84f8u},
    end_store_sites[2]{0xc856au,0xc8506u};
template<class State,class Prepare,class Settle,class Notice> struct Adapter : Port<State> {
    Prepare &prepare_child;
    Settle &settle_child;
    Notice &notice_child;
    std::uint32_t depth{};      // bytes below the return address
    bool fade_in{},timed_fade{},saved_ebx{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Prepare &p,Settle &l,Notice &n)
        :Port<State>(s,c,t,{}),prepare_child(p),settle_child(l),notice_child(n){}
    std::uint32_t local()const{return this->gp[esp]+depth-1u;}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        const unsigned in=fade_in?1u:0u;
        switch(a) {
        case weight:return weight_sites[in];
        case duration:return duration_sites[in];
        case ticks:return ticks_sites[in];
        case duration_again:return duration_again_sites[in];
        case weight_again:return weight_again_sites[in];
        case weight_store:return timed_fade?(fade_in?0xc84e4u:0xc854cu):(fade_in?0xc84efu:0xc8557u);
        case weight_final:return final_sites[in];
        case weight_end_store:return end_store_sites[in];
        default:return sites[unsigned(a)];
        }
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;auto &x=this->s.xmm;
        using enum Access;
        switch(a) {
        case headers:case instances:case game_address:case parent:gp[ecx]=v;break;
        case datums:gp[edx]=v;break;
        case object_address:gp[esi]=v;break;
        case tag:gp[eax]=fl.shift(fl.logic(v&0xffffu,4),4u,4);break;                                   // AND EAX,FFFF; SHL EAX,4
        case definition_address:gp[ebp]=v;break;
        case definition_flags:gp[edx]=v;break;
        case state:case channel_count:case second_target:case count_again:gp[eax]=v;break;
        case state_again:gp[eax]=fl.right(v,1u,4,false);fl.logic(gp[eax]&1u,1);break;                // SHR EAX,1; TEST AL,1
        case target_again:fl.minus(gp[eax],v,4);break;                                                 // CMP EAX,[ESI+248]
        case clock_address:gp[ebx]=v;break;
        case one:x[0]=lane0(v);x[1]=_mm_setzero_ps();break;                                           // MOVSS XMM0,[46A0D0]; XORPS XMM1
        case channels:gp[edx]=v;break;
        case flags_byte:set_low_byte(gp[edx],v);break;
        case weight:x[2]=lane0(v);break;
        case duration:x[2]=lane0(v);break;
        case ticks:if(fade_in)gp[eax]=logic::widen16(v);else gp[edx]=logic::widen16(v);break;
        case moved_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void framing(std::uint32_t object_handle) {                                                        // PUSH EBP, ESI, EDI; MOV EDI,EAX; AND; LEA
        auto &gp=this->gp;
        put(0xc83a0u,gp[ebp]);put(0xc83a1u,gp[esi]);put(0xc83a2u,gp[edi]);
        gp[edi]=object_handle;gp[eax]=3u*this->flags.logic(object_handle&0xffffu,4);
    }
    void fixed_tested(std::uint32_t flags) {                                                           // XOR AL,AL; SHR EDX,B; TEST DL,1
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[eax],fl.logic(0u,1));gp[edx]=fl.right(flags,logic::definition::fixed,4,false);fl.logic(gp[edx]&1u,1);
    }
    void saving_ebx(){put(0xc83d7u,this->gp[ebx]);saved_ebx=true;}
    void prepare(Kept &kept) {                                                                         // MOV EBX,EDI; CALL D3150; CALL D05A0
        auto &gp=this->gp;
        gp[ebx]=kept.clock;
        this->call(0xc83dau,0xc83dfu,0xd3150u,prepare_child);
        refresh(kept);
        this->call(0xc83dfu,0xc83e4u,0xd05a0u,settle_child);
        refresh(kept);
    }
    void state_tested(std::uint32_t state_value) {                                                     // SHR EAX,1A; TEST AL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.right(state_value,logic::object::seated_bit,4,false);fl.logic(gp[eax]&1u,1);
    }
    void kind_tested(std::uint32_t kind){this->flags.minus(kind&0xffu,logic::skipped_kind,1);}        // CMP byte [ECX+C],4
    void parent_tested(std::uint32_t parent_value){this->gp[eax]=logic::none;this->compare32(parent_value,logic::none);}   // MOV EAX,-1; CMP ECX,EAX
    void seat_tested(std::uint32_t seat_value){this->flags.minus(seat_value&0xffffu,0xffffu,2);}      // CMP word [ESI+1FC],AX
    void state_cleared(std::uint32_t cleared){this->flags.logic(cleared,4);}                          // AND dword [ESI+134],FBFFFFFF
    void notice(std::uint32_t object_handle,Kept &kept) {                                              // PUSH EDI; MOV ECX,1E; CALL E8E10
        put(0xc841cu,object_handle);this->gp[ecx]=logic::notice;
        this->call(0xc8422u,0xc8427u,0xe8e10u,notice_child);depth-=4u;
        refresh(kept);
        this->reached(Branch::noticed);
    }
    void counted(std::uint32_t count){this->gp[ecx]=this->flags.logic(0u,4);this->test32(count);}     // XOR ECX,ECX; TEST EAX,EAX
    void constants(std::uint32_t){}
    // MOV EDX,ECX; XOR AL,AL; SUB EDX,0 (DEC EDX); the targets' CMPs; MOV AL,1 or XOR AL,AL.
    void first_channel(std::uint32_t target_value,bool) {this->compare32(target_value,logic::none);}
    void second_channel(std::uint32_t second_value,bool) {this->compare32(second_value,logic::none);}
    void channel_selected(std::uint32_t index,std::uint32_t channel,bool on) {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[edi]=channel;gp[ecx]=index;
        gp[edx]=fl.minus(index,0u,4);
        if(index!=0u)gp[edx]=fl.increment(gp[edx],4,true);
        set_low_byte(gp[eax],on?1u:0u);
    }
    void held_tested(std::uint32_t byte,bool on) {                                                     // SHR DL,2; TEST DL,1; (TEST AL,AL)
        auto &gp=this->gp;auto &fl=this->flags;
        set_low_byte(gp[edx],fl.right(byte&0xffu,2u,1,false));fl.logic(gp[edx]&1u,1);
        if(!(gp[edx]&1u))this->test8(on?1u:0u);
        fade_in=!(byte&logic::object::held) && on;
    }
    void at_end(std::uint32_t weight_value,std::uint32_t end,bool) {                                   // UCOMISS XMM2,XMM0 (XMM1); LAHF; TEST AH,44
        auto &gp=this->gp;auto &fl=this->flags;
        fl.scalar_compare(lane0(weight_value),lane0(end),true);
        set_high_byte(gp[eax],fl.bits()|2u);fl.logic((gp[eax]>>8)&0x44u,1);
    }
    void timed(std::uint32_t duration_value,bool timed_value) {                                        // COMISS XMM2,XMM1
        this->flags.scalar_compare(lane0(duration_value),this->s.xmm[1],false);timed_fade=timed_value;
        this->reached(fade_in?Branch::faded_in:Branch::faded_out);
    }
    void stepped(std::uint32_t,std::uint32_t span,std::uint32_t step,std::uint32_t moved_weight,bool in) {
        auto &x=this->s.xmm;
        x[3]=lane0(step);
        x[2]=lane0(in?span:moved_weight);
        if(in)x[3]=lane0(moved_weight);
    }
    void ended(std::uint32_t final_weight,bool past,bool in) {                                         // COMISS XMM2,XMM0 or COMISS XMM1,[weight]
        auto &x=this->s.xmm;auto &fl=this->flags;
        if(in){x[2]=lane0(final_weight);fl.scalar_compare(x[2],x[0],false);}
        else fl.scalar_compare(x[1],lane0(final_weight),false);
        if(past)this->reached(Branch::stopped);
    }
    void advanced(std::uint32_t index,std::uint32_t count){this->gp[ecx]=this->flags.increment(index-1u,4,false);this->compare32(index,count);}
    void restoring(){this->gp[ebx]=take(0xc858bu);saved_ebx=false;}
    void run() {
        auto &gp=this->gp;
        put(0xc8390u,gp[ecx]);
        Memory<Adapter> memory{*this};
        Outcome outcome{};
        const auto moved=logic::channel_fades::update(memory,*this,gp[eax],outcome);
        this->reached(unsigned(outcome));
        set_low_byte(gp[eax],moved);
        gp[edi]=take(0xc858cu);gp[esi]=take(0xc858du);gp[ebp]=take(0xc858eu);gp[ecx]=take(0xc858fu);
        this->ret(0xc8590u,0u);this->publish(0xc8590u);
    }
};
template<class State,class Reference,class Prepare,class Settle,class Notice>
bool bridge(State &s,Reference reference,Prepare prepare,Settle settle,Notice notice) {
    return run_owner(s,0xc8390u,0xc8590u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Prepare,Settle,Notice>(v,c,t,prepare,settle,notice);});
}
} // namespace halo2::native_channel_fades_detail
