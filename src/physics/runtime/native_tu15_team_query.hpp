#pragma once
// Boundary adapter for the team query, entered at 15C7F0
// (native_tu15_team_query_systems.hpp). Its callers keep the original ABI,
// which the adapter owns: the team at [ESP+4], RET 4; ESI saved (the team's
// bit); the result in AL (XOR AL,AL, MOV AL,1) above the rest of EAX as the
// original leaves it; ECX and EDX as the original leaves them (the mask or
// the variant, the game globals); the flags of the last instruction that
// sets them. Each access publishes its original instruction.
// HALO2_REFERENCE_TEAM_QUERY=1 restores the translated body.
// HALO2_VERIFY_TEAM_QUERY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_team_query_systems.hpp"

namespace halo2::native_team_query_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_team_query;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TEAM_QUERY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TEAM_QUERY");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_variant, other_mode, other_team, unflagged, takes_part, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Team query verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State> struct TeamQuery : Port<State> {
    using Access=logic::team_query::Access;
    TeamQuery(State &s,std::uint32_t *c,Trace<State> *t):Port<State>(s,c,t,{}){}
    std::uint32_t argument()const{return this->gp[esp]+4u;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a) {
        case session:return 0x15c7f0u;
        case variant_index:return 0x15c7f6u;
        case variant:return 0x15c7fcu;
        case game_state:return 0x15c809u;
        case mode:return 0x15c80fu;
        case team:return 0x15c818u;
        case mask_address:return 0x15c824u;
        case mask:return 0x15c82au;
        case flags:return 0x15c832u;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case session:case mask_address:gp[ecx]=v;break;
        case variant_index:case game_state:gp[edx]=v;break;
        // MOV ECX,[...]; XOR AL,AL; TEST ECX,ECX.
        case variant:gp[ecx]=v;set_low_byte(gp[eax],fl.logic(0u,1));this->test32(v);if(!v)this->reached(Branch::no_variant);break;
        case mode:fl.minus(v,logic::wanted_mode,4);if(v!=logic::wanted_mode)this->reached(Branch::other_mode);break;   // CMP [EDX+180],7
        // MOV CL,[ESP+4]; PUSH ESI; MOV ESI,1; SHL ESI,CL.
        case team:set_low_byte(gp[ecx],v);this->push(0x15c81cu,gp[esi]);gp[esi]=fl.shift(1u,v&0xffu,4);break;
        // MOVZX ECX,word [ECX]; TEST ECX,ESI; POP ESI.
        case mask:gp[ecx]=v&0xffffu;this->test32(gp[ecx]&gp[esi]);
            if(!(gp[ecx]&gp[esi]))this->reached(Branch::other_team);gp[esi]=this->pop(0x15c82fu);break;
        case flags:this->test8(v&logic::team_flag);this->reached((v&logic::team_flag)?Branch::takes_part:Branch::unflagged);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    void run() {
        Memory<TeamQuery> memory{*this};
        if(logic::team_query::takes_part(memory,*this))set_low_byte(this->gp[eax],1u);              // MOV AL,1
        this->ret(0x15c83du,4u);this->publish(0x15c83du);
    }
};
template<class State,class Reference> bool bridge(State &s,Reference reference) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x15c7f0u,0x15c83du,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TeamQuery<State>(v,c,t);});
    return true;
}
} // namespace halo2::native_team_query_detail
