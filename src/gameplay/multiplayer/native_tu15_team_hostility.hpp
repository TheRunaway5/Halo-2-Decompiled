#pragma once
// Boundary adapter for the team hostility, entered at 1E7810
// (native_tu15_team_hostility_systems.hpp). Its callers and the children
// keep the original ABI, which the adapter owns: CX and SI the teams; C1E90
// gets EDX the bit index and the bits' address pushed, 160A60 (through the
// unit's dispatch) the teams pushed, each pops its arguments, and every
// register each returns comes back; the result in AL (MOV AL,1; NEG, SBB,
// INC of C1E90's answer; 160A60's; SETNE) above the rest of EAX; ECX and EDX
// as the original leaves them; the flags of the last instruction that sets
// them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_TEAM_HOSTILITY=1 restores the translated body.
// HALO2_VERIFY_TEAM_HOSTILITY=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "gameplay/multiplayer/native_tu15_team_hostility_systems.hpp"

namespace halo2::native_team_hostility_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_team_hostility;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_TEAM_HOSTILITY");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_TEAM_HOSTILITY");return p && *p;}();
    return value;
}
enum Branch : unsigned {no_team, out_of_range, allied, decided, other_mode, branch_count};
static_assert(unsigned(logic::team_hostility::Outcome::other_mode)==Branch::other_mode);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Team hostility verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
template<class State,class Allied,class Decide> struct TeamHostility : Port<State> {
    using Access=logic::team_hostility::Access;
    Allied &allied_child;
    Decide &decide_route;
    TeamHostility(State &s,std::uint32_t *c,Trace<State> *t,Allied &a,Decide &d):Port<State>(s,c,t,{}),allied_child(a),decide_route(d){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        using enum Access;
        switch(a){case game_state:return 0x1e781eu;case mode:return 0x1e7824u;
            case alliances:this->gp[edx]=logic::widen16(this->gp[ecx]);return 0x1e7845u;}                // MOVSX EDX,CX
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;auto &fl=this->flags;
        using enum Access;
        switch(a) {
        case game_state:gp[edx]=v;break;
        // MOV EDX,[EDX+8]; CMP EDX,1 (then CMP EDX,2 past it).
        case mode:gp[edx]=v;fl.minus(v,logic::alliance_mode,4);if(v!=logic::alliance_mode)fl.minus(v,logic::decided_mode,4);break;
        // MOV ECX,[5020C4]; MOVSX EAX,SI.
        case alliances:gp[ecx]=v;gp[eax]=logic::widen16(gp[esi]);break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // TEST team,team; then CMP team,10 unless negative.
    void ranged(std::uint32_t value) {
        auto &fl=this->flags;
        fl.logic(value&0xffffu,2);if(!(value&0x8000u))fl.minus(value&0xffffu,logic::teams,2);
    }
    // SHL EDX,4; ADD ECX,C4; ADD EDX,EAX; PUSH ECX; CALL C1E90; NEG AL; SBB AL,AL; INC AL.
    bool allied(std::uint32_t index,std::uint32_t bits) {
        auto &gp=this->gp;auto &fl=this->flags;
        const auto shifted=fl.shift(gp[edx],4u,4);gp[ecx]=fl.plus(gp[ecx],logic::alliance_bits,4);fl.plus(shifted,gp[eax],4);
        gp[edx]=index;
        this->push(0x1e7859u,bits);this->call(0x1e785au,0x1e785fu,0xc1e90u,allied_child);
        const auto answer=gp[eax]&0xffu;
        const auto negated=fl.minus(0u,answer,1);
        set_low_byte(gp[eax],fl.increment(fl.borrow(negated,negated,1),1,false));
        return answer!=0;
    }
    // PUSH ESI; PUSH ECX; CALL 160A60.
    std::uint32_t decide(std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        this->push(0x1e786bu,gp[esi]);this->push(0x1e786cu,gp[ecx]);this->call_routed(decide_route,0x1e786du,0x1e7872u,0x160a60u);
        return gp[eax]&0xffu;
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        using Outcome=logic::team_hostility::Outcome;
        // CMP CX,-1; MOV AL,1; then CMP SI,-1.
        fl.minus(gp[ecx]&0xffffu,0xffffu,2);set_low_byte(gp[eax],1u);
        if((gp[ecx]&0xffffu)!=0xffffu)fl.minus(gp[esi]&0xffffu,0xffffu,2);
        Memory<TeamHostility> memory{*this};
        const auto result=logic::team_hostility::hostile(memory,*this,gp[ecx],gp[esi]);
        this->reached(unsigned(result.outcome));
        std::uint32_t exit=0x1e787cu;
        switch(result.outcome) {
        case Outcome::no_team:exit=0x1e787cu;break;                                                   // MOV AL,1 at 1E787A
        case Outcome::out_of_range:exit=0x1e787cu;break;
        case Outcome::allied:exit=0x1e7865u;break;
        case Outcome::decided:exit=0x1e7872u;break;
        case Outcome::other_mode:fl.minus(gp[ecx]&0xffffu,gp[esi]&0xffffu,2);exit=0x1e7879u;break;  // CMP CX,SI; SETNE AL
        }
        set_low_byte(gp[eax],result.value);
        this->ret(exit,0);this->publish(exit);
    }
};
template<class State,class Reference,class Allied,class Decide> bool bridge(State &s,Reference reference,Allied allied,Decide decide) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1e7810u,0x1e787cu,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return TeamHostility<State,Allied,Decide>(v,c,t,allied,decide);});
    return true;
}
} // namespace halo2::native_team_hostility_detail
