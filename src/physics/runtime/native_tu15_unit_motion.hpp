#pragma once
// Boundary adapter for the unit motion check, entered at E0500
// (native_tu15_unit_motion_systems.hpp). Its callers and the child keep the
// original ABI, which the adapter owns: ESI the handle; AL the result; EBX
// saved (BL the support), EDI saved around the cluster's data; E65A0 gets
// EAX the handle, and every register it returns comes back; the speed runs
// on the logical x87 as the original's FLD m32 (three), FLD/FMUL/FADDP,
// FABS, FSTP ST3, two FSTP ST0, FLD m32, FCOMIP and FSTP ST0 do (five free
// registers are required at admission); the flags of the last instruction
// that sets them; RET. Each access publishes its original instruction.
// HALO2_REFERENCE_UNIT_MOTION=1 restores the translated body.
// HALO2_VERIFY_UNIT_MOTION=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "physics/runtime/native_tu15_object_tick.hpp"
#include "physics/runtime/native_tu15_unit_motion_systems.hpp"
#include <cmath>

namespace halo2::native_unit_motion_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
namespace logic=halo2::native_unit_motion;
inline bool reference_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_REFERENCE_UNIT_MOTION");return p && *p;}();
    return value;
}
inline bool verify_mode() {
    static const bool value=[]{const auto *p=std::getenv("HALO2_VERIFY_UNIT_MOTION");return p && *p;}();
    return value;
}
enum Branch : unsigned {frozen, no_cluster, unflagged, detached, supported, moving_state, supported_state, fast,
    active_state, ready, not_ready, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> value{0};return value;}
struct VerifySummary : native_object_tick_detail::Tally {
    std::atomic<std::uint64_t> calls{};
    ~VerifySummary() {
        if(calls)std::fprintf(stderr,"Unit motion verify: %llu calls; %llu ordered accesses (%llu stores) identical, "
            "coverage %llx\n",(unsigned long long)calls.load(),(unsigned long long)accesses.load(),
            (unsigned long long)stores.load(),(unsigned long long)coverage().load());
    }
};
inline VerifySummary &summary(){static VerifySummary value;return value;}
inline constexpr Edge motion_edges[]{
    {0xe051eu,0xe05c6u,frozen},{0xe0529u,0xe0562u,no_cluster},{0xe0552u,0xe0562u,unflagged},
    {0xe0555u,0xe0562u,detached},
};
inline long double extended(std::uint32_t bits){return physics_step_detail::ext(physics_step_detail::f(bits));}
template<class State,class Ready> struct Motion : Port<State> {
    using Access=logic::unit_motion::Access;
    Ready &ready_child;
    Motion(State &s,std::uint32_t *c,Trace<State> *t,Ready &r):Port<State>(s,c,t,motion_edges),ready_child(r){}
    std::uint32_t instruction(Access a,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        // MOV ECX,ESI; AND ECX,FFFF; LEA EAX,[ECX+ECX*2].
        case object_table:gp[ecx]=this->flags.logic(gp[esi]&0xffffu,4);gp[eax]=gp[ecx]*3u;return 0xe050bu;
        case object_datums:return 0xe0511u;
        case object_address:return 0xe0514u;
        case frozen:return 0xe0518u;
        case cluster:return 0xe0529u;
        case clusters:return 0xe0534u;
        // AND ECX,FFFF; LEA ECX,[ECX+ECX*4]; PUSH EDI.
        case cluster_data:
            gp[ecx]=this->flags.logic(gp[ecx]&0xffffu,4)*5u;this->push(0xe0542u,gp[edi]);return 0xe0543u;
        case surface:return 0xe0549u;
        case flagged:return 0xe054fu;
        case cluster_flags:return 0xe0555u;
        case state:return 0xe0562u;
        case velocity_z:return 0xe0572u;
        case velocity_y:return 0xe0578u;
        case velocity_x:return 0xe057eu;
        case still_speed:return 0xe059cu;
        }
        return 0;
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        auto &fp=this->s.fp;
        using enum Access;
        switch(a) {
        case object_table:gp[ecx]=v;break;
        case object_datums:case object_address:gp[edx]=v;break;
        // PUSH EBX; XOR BL,BL; TEST AL,AL.
        case frozen:
            set_low_byte(gp[eax],v);this->push(0xe051eu,gp[ebx]);set_low_byte(gp[ebx],0u);this->test8(v);
            if(v&0xffu)this->reached(Branch::frozen);
            break;
        case cluster:gp[ecx]=v;this->compare32(v,logic::none);break;
        case clusters:gp[eax]=v;break;
        // SHL ECX,5; ... ADD ECX,EDI.
        case cluster_data:gp[edi]=v;gp[ecx]=this->flags.shift(gp[ecx],5u,4);break;
        case surface:gp[eax]=v;gp[ecx]=this->flags.plus(gp[ecx],gp[edi],4);break;
        // CMP byte,BL; POP EDI.
        case flagged:this->flags.minus(v&0xffu,gp[ebx]&0xffu,1);gp[edi]=this->pop(0xe0552u);break;
        // SHR ECX,12; TEST CL,1; (MOV BL,1).
        case cluster_flags:
            gp[ecx]=this->flags.right(v,logic::cluster::detached_bit,4,false);this->test8(gp[ecx]&1u);break;
        // MOV CL; CMP CL,6; (CMP CL,3).
        case state:
            set_low_byte(gp[ecx],v);this->flags.minus(v&0xffu,logic::unit::moving_state,1);
            if((v&0xffu)!=logic::unit::moving_state)this->flags.minus(v&0xffu,logic::unit::supported_state,1);
            if((v&0xffu)==logic::unit::moving_state)this->reached(Branch::moving_state);
            else if((v&0xffu)==logic::unit::supported_state)this->reached(Branch::supported_state);
            break;
        case velocity_z:case velocity_y:case velocity_x:fp.push(extended(v));break;
        // FLD ST0; FMUL ST,ST1; FLD ST2; FMUL ST,ST3; FADDP; FLD ST3; FMUL ST,ST4; FADDP; FABS; FSTP ST3;
        // FSTP ST0; FSTP ST0; FLD m32.
        case still_speed: {
            fp.push(fp.st(0));fp.st(0)=physics_step_detail::mul(fp.st(0),fp.st(1));
            fp.push(fp.st(2));fp.st(0)=physics_step_detail::mul(fp.st(0),fp.st(3));
            fp.st(1)=physics_step_detail::add(fp.st(1),fp.st(0));fp.pop();
            fp.push(fp.st(3));fp.st(0)=physics_step_detail::mul(fp.st(0),fp.st(4));
            fp.st(1)=physics_step_detail::add(fp.st(1),fp.st(0));fp.pop();
            fp.st(0)=std::fabs(fp.st(0));
            fp.st(3)=fp.st(0);fp.pop();fp.st(0)=fp.st(0);fp.pop();fp.st(0)=fp.st(0);fp.pop();
            fp.push(extended(v));break;
        }
        default:break;
        }
    }
    void stored(Access,std::uint32_t,std::uint32_t){}
    // FCOMIP ST,ST1; FSTP ST0; JBE: the still speed not above the squared speed (or unordered);
    // (MOVZX ECX,CL; CMP ECX,4; CMP ECX,5).
    bool faster_than_still() {
        auto &fp=this->s.fp;
        this->s.flags=this->flags;this->s.compare_and_pop(fp.st(1));this->flags=this->s.flags;
        fp.st(0)=fp.st(0);fp.pop();
        const bool fast_enough=this->flags.cf || this->flags.zf;
        if(fast_enough){this->reached(Branch::fast);return true;}
        auto &gp=this->gp;
        gp[ecx]&=0xffu;this->compare32(gp[ecx],logic::unit::first_active_state);
        if(gp[ecx]!=logic::unit::first_active_state)this->compare32(gp[ecx],logic::unit::second_active_state);
        if(gp[ecx]==logic::unit::first_active_state || gp[ecx]==logic::unit::second_active_state)this->reached(Branch::active_state);
        return false;
    }
    void supported(){set_low_byte(this->gp[ebx],1u);this->reached(Branch::supported);}         // MOV BL,1
    std::uint32_t ready() {
        auto &gp=this->gp;
        gp[eax]=gp[esi];this->call(0xe05b7u,0xe05bcu,0xe65a0u,ready_child);this->test8(gp[eax]);   // MOV EAX,ESI; CALL; TEST AL,AL
        this->reached(gp[eax]&0xffu?Branch::ready:Branch::not_ready);
        return gp[eax];
    }
    void run() {
        auto &gp=this->gp;
        Memory<Motion> memory{*this};
        // AL=1 at E05C0, or AL=BL (the support) at E05C4.
        if(logic::unit_motion::moving(memory,*this,gp[esi])==logic::unit_motion::Exit::moving){set_low_byte(gp[eax],1u);gp[ebx]=this->pop(0xe05c2u);this->ret(0xe05c3u,0);this->publish(0xe05c3u);}
        else{set_low_byte(gp[eax],gp[ebx]);gp[ebx]=this->pop(0xe05c6u);this->ret(0xe05c7u,0);this->publish(0xe05c7u);}
    }
};
template<class State,class Reference,class Ready> bool bridge(State &s,Reference reference,Ready ready) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.fp.count>3u)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0xe0500u,0xe05c7u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Motion<State,Ready>(v,c,t,ready);});
    return true;
}
} // namespace halo2::native_unit_motion_detail
