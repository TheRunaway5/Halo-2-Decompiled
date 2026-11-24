#pragma once
// Boundary adapter for the state reset, entered at C93D0
// (native_tu15_state_reset_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: EDX the state; EBX, EBP and EDI saved (the copies
// run through them); the clear runs as the original's REP STOSD (the
// direction flag clear: a set one is refused at admission); EAX, ECX and
// XMM0 as the original leaves them; the flags of the last instruction that
// sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_STATE_RESET=1 restores the translated body.
// HALO2_VERIFY_STATE_RESET=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "campaign/ai/native_tu15_state_reset_systems.hpp"
#include <emmintrin.h>

namespace halo2::native_state_reset_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_state_reset;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_STATE_RESET");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_STATE_RESET");return p && *p;}();
    return value;
}
enum Branch : unsigned {reset, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"State reset verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct Reset : Port<State> {
    using Access=logic::state_reset::Access;
    std::uint32_t point{},word{};
    Reset(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        static constexpr std::uint32_t reads[3][3]{{0xc9404u,0xc940bu,0xc9411u},{0xc941cu,0xc9423u,0xc9429u},{0xc942fu,0xc9436u,0xc943cu}},
            writes[3][3]{{0xc9409u,0xc940eu,0xc9414u},{0xc9421u,0xc9426u,0xc942cu},{0xc9434u,0xc9439u,0xc943fu}};
        using enum Access;
        switch(a) {
        case clear:return 0xc93dcu;
        case index_store:return 0xc93deu;
        case origin:return 0xc93e2u;
        case kind_store:return 0xc93e7u;
        case handle_a:return 0xc93f0u;
        case handle_b:return 0xc93f4u;
        case handle_c:return 0xc93f7u;
        case handle_d:return 0xc93fau;
        case handle_e:return 0xc93feu;
        case point_read:return reads[point][word];
        case point_store:return writes[point][word++];
        case first_distance:return 0xc9444u;
        case second_distance:return 0xc9449u;
        case handle_f:return 0xc944eu;
        case handle_g:return 0xc9451u;
        case handle_h:return 0xc9454u;
        case count:return 0xc9457u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==origin)gp[eax]=v;
        // MOV EBP (words 0, 1) or MOV EBX/EAX (word 2) from the point.
        else if(a==point_read)gp[word==2u?(point==2u?eax:ebx):(point==2u?ebx:ebp)]=v;
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        if(a==clear){gp[edi]+=4u;--gp[ecx];}
        // OR ECX,-1 after the kind.
        else if(a==kind_store)gp[ecx]=this->flags.logic(gp[ecx]|logic::none,4);
        // POP EDI; POP EBP after the last point.
        else if(a==point_store && point==2u && word==3u){gp[edi]=this->pop(0xc9442u);gp[ebp]=this->pop(0xc9443u);}
    }
    // XOR EAX,EAX; MOV ECX,1F; MOV EDI,EDX; REP STOSD (before), then nothing.
    void cleared(){}
    // MOV EBX,EAX; LEA EDI,[EDX+point] (XORPS XMM0 before the second); the third copies through EBX from EAX.
    void point_starts(std::uint32_t offset) {
        auto &gp=this->gp;
        point=offset==logic::state::first_point?0u:offset==logic::state::second_point?1u:2u;word=0;
        if(point==1u)this->s.xmm[0]=_mm_setzero_ps();
        if(point<2u)gp[ebx]=gp[eax];
        gp[edi]=gp[edx]+offset;
    }
    void run() {
        auto &gp=this->gp;
        this->push(0xc93d0u,gp[ebx]);this->push(0xc93d1u,gp[ebp]);this->push(0xc93d2u,gp[edi]);
        gp[eax]=this->flags.logic(0u,4);gp[ecx]=logic::state_words;gp[edi]=gp[edx];
        Memory<Reset> memory{*this};
        struct Hooks {
            Reset &r;
            void cleared(){r.cleared();}
            void point(std::uint32_t offset){r.point_starts(offset);}
        } hooks{*this};
        logic::state_reset::reset(memory,hooks,gp[edx]);
        this->reached(Branch::reset);
        gp[ebx]=this->pop(0xc945du);this->ret(0xc945eu,0);this->publish(0xc945eu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.other_flags&0x400u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xc93d0u,0xc945eu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Reset<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_state_reset_detail
