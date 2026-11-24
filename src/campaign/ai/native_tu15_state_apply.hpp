#pragma once
// Boundary adapter for the state apply, entered at C92C0
// (native_tu15_state_apply_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EAX the unit handle (then the unit), ECX the
// state; ESI and EDI saved (the copies run through EDX, EDI and ESI); the
// tail runs as the original's REP MOVSD (the direction flag clear: a set
// one is refused at admission); the flags of the last CMP; RET. Each access
// publishes its original instruction.
// HALO2_REFERENCE_STATE_APPLY=1 restores the translated body.
// HALO2_VERIFY_STATE_APPLY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "campaign/ai/native_tu15_state_apply_systems.hpp"

namespace halo2::native_state_apply_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_state_apply;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_STATE_APPLY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_STATE_APPLY");return p && *p;}();
    return value;
}
enum Branch : unsigned {pair_kept, pair_copied, no_choice, choice_copied, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"State apply verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge apply_edges[]{
    {0xc930fu,0xc9321u,pair_kept},{0xc930fu,0xc9318u,pair_copied},{0xc9321u,0xc9331u,no_choice},
    {0xc9321u,0xc9328u,choice_copied},
};
template<class State> struct Apply : Port<State> {
    using Access=logic::state_apply::Access;
    using Point=logic::state_apply::Point;
    std::uint32_t unit{};unsigned word{};
    Apply(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,apply_edges){}
    // The point copies: (read, store) PCs of each word.
    static std::uint32_t point_pc(Access a,unsigned w) {
        using enum Access;
        static constexpr std::uint32_t position_pc[]{0xc92dau,0xc92e2u,0xc92e4u,0xc92e7u,0xc92eau,0xc92edu},
            goal_pc[]{0xc9346u,0xc934eu,0xc9350u,0xc9353u,0xc9356u,0xc9359u},
            facing_pc[]{0xc935fu,0xc9367u,0xc9369u,0xc936cu,0xc936fu,0xc9372u},
            origin_pc[]{0xc9378u,0xc9380u,0xc9382u,0xc9385u,0xc9388u,0xc938bu},
            aim_pc[]{0xc9399u,0xc93a1u,0xc93a3u,0xc93a6u,0xc93a9u,0xc93acu};
        const auto store=a==position_store||a==goal_store||a==facing_store||a==origin_store||a==aim_store;
        const auto *table=a==position_read||a==position_store?position_pc:a==goal_read||a==goal_store?goal_pc:
            a==facing_read||a==facing_store?facing_pc:a==origin_read||a==origin_store?origin_pc:aim_pc;
        return table[w*2u+(store?1u:0u)];
    }
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:return 0xc92c0u;
        case object_datums:return 0xc92c6u;
        // AND EAX,FFFF; PUSH ESI; LEA EAX,[EAX+EAX*2].
        case object_address:gp[eax]=this->flags.logic(gp[eax]&0xffffu,4);this->push(0xc92ceu,gp[esi]);gp[eax]*=3u;return 0xc92d2u;
        case position_read:case goal_read:case facing_read:case origin_read:case aim_read:return point_pc(a,word);
        case position_store:case goal_store:case facing_store:case origin_store:case aim_store:return point_pc(a,word++);
        case timer_a_read:return 0xc92f0u;
        case timer_a_store:return 0xc92f3u;
        case timer_b_read:return 0xc92f9u;
        case timer_b_store:return 0xc92fcu;
        case mode_read:return 0xc9302u;
        case mode_store:return 0xc9305u;
        case pair_word:return 0xc930bu;
        case pair_current:return 0xc930fu;
        case pair_read:return 0xc9318u;
        case pair_store:return 0xc931bu;
        case choice_check:return 0xc9321u;
        case choice_read:return 0xc9328u;
        case choice_store:return 0xc932bu;
        case flag_read:return 0xc9331u;
        case flag_store:return 0xc9334u;
        case link_read:return 0xc933au;
        case link_store:return 0xc933du;
        case kind_read:return 0xc938eu;
        case kind_store:return 0xc9390u;
        case tail_read:case tail_store:return 0xc93bdu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_table:case object_datums:gp[edx]=v;break;
        case object_address:gp[eax]=v;unit=v;break;
        // MOV EDI,[EDX] and [EDX+4]; MOV EDX,[EDX+8].
        case position_read:case goal_read:case facing_read:case origin_read:case aim_read:gp[word==2u?edx:edi]=v;break;
        case timer_a_read:case timer_b_read:case pair_read:case link_read:case kind_read:gp[edx]=v;break;
        case mode_read:case choice_read:case flag_read:set_low_byte(gp[edx],v);break;
        case pair_word:gp[edx]=(gp[edx]&0xffff0000u)|(v&0xffffu);break;
        case pair_current:this->flags.minus(gp[edx]&0xffffu,v&0xffffu,2);break;                   // CMP DX,word
        case choice_check:this->flags.minus(v&0xffffu,0xffffu,2);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        if(a==tail_store){gp[esi]+=4u;gp[edi]+=4u;--gp[ecx];}
    }
    // LEA EDX,[ECX+from] (PUSH EDI before the first); LEA ESI,[EAX+to].
    void point(const Point &p) {
        auto &gp=this->gp;
        if(p.read==Access::position_read)this->push(0xc92d9u,gp[edi]);
        gp[edx]=gp[ecx]+p.from;gp[esi]=gp[eax]+p.to;word=0;
    }
    // LEA ESI,[ECX+58]; LEA EDI,[EAX+1C8]; MOV ECX,9.
    void tail(){auto &gp=this->gp;gp[esi]=gp[ecx]+0x58u;gp[edi]=gp[eax]+0x1c8u;gp[ecx]=logic::tail_words;}
    void run() {
        auto &gp=this->gp;
        Memory<Apply> memory{*this};
        logic::state_apply::apply(memory,*this,gp[eax],gp[ecx]);
        gp[edi]=this->pop(0xc93bfu);gp[esi]=this->pop(0xc93c0u);this->ret(0xc93c1u,0);this->publish(0xc93c1u);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc92c0u,0xc93c1u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Apply<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_state_apply_detail
