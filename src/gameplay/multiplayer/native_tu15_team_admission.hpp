#pragma once
// Boundary adapter for the team admission, entered at 161560
// (native_tu15_team_admission_systems.hpp). Its callers keep the original
// ABI, which the adapter owns: ECX the team; ESI and EDI saved; the result
// in AL (MOV AL,DL or SETNE) above the rest of EAX (the game globals once
// read); ECX (the mask) and EDX (1) as the original leaves them; the flags
// of the last instruction that sets them; RET. Each access publishes its
// original instruction.
// HALO2_REFERENCE_TEAM_ADMISSION=1 restores the translated body.
// HALO2_VERIFY_TEAM_ADMISSION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/multiplayer/native_tu15_team_admission_systems.hpp"

namespace halo2::native_team_admission_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_team_admission;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TEAM_ADMISSION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TEAM_ADMISSION");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_variant, no_team, admitted, decided, branch_count};
static_assert(unsigned(logic::team_admission::Outcome::decided)==Branch::decided);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Team admission verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct TeamAdmission : Port<State> {
    using Access=logic::team_admission::Access;
    TeamAdmission(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case session:this->push(0x161560u,this->gp[esi]);return 0x161561u;                         // PUSH ESI
        case variant_index:return 0x161567u;
        case variant:this->push(0x16156du,this->gp[edi]);return 0x16156eu;                         // PUSH EDI
        case game_state:return 0x161585u;
        case mode:return 0x16158au;
        case mask_address:return 0x161597u;
        case mask:return 0x16159du;
        case flags:return 0x1615a4u;
        case closed:return 0x1615b2u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case session:case variant_index:gp[esi]=v;break;
        // MOV EDI,[56DFA0+ESI*4]; TEST EDI,EDI; MOV EDX,1; MOV AL,DL; then CMP ECX,-1.
        case variant:gp[edi]=v;this->test32(v);gp[edx]=1u;set_low_byte(gp[eax],1u);if(v)fl.minus(gp[ecx],logic::none,4);break;
        case game_state:gp[eax]=v;break;
        case mode:fl.minus(v,logic::wanted_mode,4);break;                                           // CMP [EAX+180],7
        case mask_address:gp[ecx]=v;break;
        case mask:gp[ecx]=v&0xffffu;this->test32(gp[ecx]&gp[esi]);break;                            // MOVZX ECX,word; TEST ECX,ESI
        case flags:this->test8(v&logic::team_flag);break;                                           // TEST byte [EAX+22C],2
        case closed:fl.minus(v,gp[edx],4);break;                                                    // CMP [EAX+1C4],EDX
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // MOV ESI,EDX; SHL ESI,CL.
    std::uint32_t masked(std::uint32_t team) {
        auto &gp=this->gp;
        gp[esi]=this->flags.shift(gp[edx],team&0xffu,4);
        return gp[esi];
    }
    void run() {
        auto &gp=this->gp;
        using Outcome=logic::team_admission::Outcome;
        Memory<TeamAdmission> memory{*this};
        const auto result=logic::team_admission::admits(memory,*this,gp[ecx]);
        this->reached(unsigned(result.outcome));
        set_low_byte(gp[eax],result.value);                                                          // MOV AL,DL or SETNE AL
        if(result.outcome==Outcome::admitted) {
            gp[edi]=this->pop(0x1615adu);gp[esi]=this->pop(0x1615b0u);this->ret(0x1615b1u,0);this->publish(0x1615b1u);return;
        }
        gp[edi]=this->pop(0x1615bbu);gp[esi]=this->pop(0x1615bcu);this->ret(0x1615bdu,0);this->publish(0x1615bdu);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x161560u,0x1615bdu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TeamAdmission<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_team_admission_detail
